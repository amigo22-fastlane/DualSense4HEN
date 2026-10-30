"""Synthetic log records test evidence handling; never stand in for HID reports."""
from argparse import Namespace
import ftplib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from probe_log import appended_log
from usb_owner_log import assess_usb_owner, BOOT
import usb_owner_test as runner


def transaction(code=0, count=1, driver='uhid0', active=0, name=0, opened=9,
                identity=0, product='0ce6', close=0, target=True):
    header = (f'{BOOT}\nexecution context=PayLoader host; log=/data/dualsense4hen-probe.log\n'
              'USB owner revision=1 firmware=0x12520001; RO query only\n'
              'load /system/common/lib/libSceUsbd.sprx handle=0x20 start_result=0x0\n'
              'USB owner ABI profile verified: 65536-byte 12.52 RX, CRC(A1)=0x3efeb3ed\n'
              'USB owner Init=0x0\n'
              f'USB owner count={count}\n')
    if target:
        header += ('USB owner identified 054c:0ce6 bus=0 address=2 iface=3\n'
                   f'USB owner open={opened} errno=0 identity_ret={identity} errno=0 vid=054c pid={product}\n'
                   f'USB owner ACTIVE raw={active} errno=6 NAME raw={name} errno=1 driver={driver} close={close} result={code}\n')
    return (header + f'USB owner complete={code}; ACTIVE/name never proves cause\nprobe exit={code}\n'
            'unload owned module=0x20 ret=0x0 result=0x0\n').encode()


class UsbOwnerEvidenceTests(unittest.TestCase):
    def test_named_driver_never_proves_input_or_cause(self):
        result = assess_usb_owner(transaction())
        self.assertEqual(result['status'], 'DRIVER_NAME_REPORTED')
        self.assertTrue(result['complete'])
        self.assertFalse(result['physical_input_proven'])
        self.assertEqual(result['cause_0x802400ff'], 'NOT_DETERMINED')

    def test_empty_success_is_not_proof_of_no_driver(self):
        result = assess_usb_owner(transaction(driver=''))
        self.assertEqual(result['status'], 'EMPTY_DRIVER_NAME')
        self.assertEqual(result['binding'], 'EMPTY_NAME_NOT_PROOF_OF_NO_DRIVER')

    def test_partial_query_preserves_name_and_raw_errors(self):
        result = assess_usb_owner(transaction(code=-4, active=-1))
        self.assertEqual(result['status'], 'METADATA_QUERY_REFUSED')
        self.assertEqual(result['binding'], 'DRIVER_NAME_REPORTED')
        self.assertEqual(result['active_errno'], 6)
        result = assess_usb_owner(transaction(code=-4, name=-1, driver=''))
        self.assertEqual(result['binding'], 'UNKNOWN')

    def test_open_identity_and_close_failures(self):
        for kwargs, status in (({'code': -2, 'opened': -1, 'identity': -1}, 'NODE_OPEN_REFUSED'),
                               ({'code': -3, 'identity': -1}, 'IDENTITY_QUERY_FAILED'),
                               ({'code': -3, 'product': '05c4'}, 'IDENTITY_CHANGED_OR_INVALID'),
                               ({'code': -5, 'close': -1}, 'NODE_CLOSE_FAILED')):
            with self.subTest(status=status):
                self.assertEqual(assess_usb_owner(transaction(**kwargs))['status'], status)

    def test_enumeration_failures_are_not_controller_absence(self):
        for count, status in ((0, 'ENUMERATION_EMPTY'), (-1, 'ENUMERATION_FAILED'),
                              (257, 'ENUMERATION_LIMIT_REFUSED'), (2, 'TARGET_NOT_IDENTIFIED')):
            self.assertEqual(assess_usb_owner(transaction(code=-17, count=count, target=False))['status'], status)

    def test_waits_for_cleanup_logged_after_exit(self):
        raw = transaction()
        pending = raw[:raw.index(b'unload owned')]
        self.assertFalse(assess_usb_owner(pending)['complete'])
        self.assertEqual(assess_usb_owner(pending)['status'], 'WAITING_FOR_MODULE_CLEANUP')
        failed = raw.replace(b'ret=0x0 result=0x0', b'ret=0x1 result=0x0')
        self.assertEqual(assess_usb_owner(failed)['status'], 'MODULE_CLEANUP_FAILED')

    def test_stale_and_mixed_transactions_rejected(self):
        old = transaction()
        self.assertEqual(assess_usb_owner(appended_log(old, old))['status'], 'NO_NEW_RUN')
        self.assertEqual(assess_usb_owner(old + transaction(code=-4, name=-1))['status'], 'AMBIGUOUS_OR_UNFRAMED_LOG')
        self.assertEqual(assess_usb_owner(old.replace(b'probe exit=0', b'probe exit=-4'))['status'], 'MISSING_OR_INCONSISTENT_SUMMARY')

    def test_guard_refusals_and_incomplete_boot(self):
        self.assertEqual(assess_usb_owner(BOOT + '\nprobe exit=-11\n')['status'], 'REFUSED_BEFORE_USB_START')
        self.assertFalse(assess_usb_owner(BOOT)['complete'])
        raw = (BOOT + '\nUSB owner revision=1 firmware=0x12520001; RO query only\n'
               'load /system/common/lib/libSceUsbd.sprx handle=0x20 start_result=0x0\n'
               'probe exit=-13\nunload owned module=0x20 ret=0x0 result=0x0\n')
        self.assertEqual(assess_usb_owner(raw)['status'], 'LIBRARY_PROFILE_REFUSED')
        self.assertEqual(assess_usb_owner(transaction(code=-13))['status'], 'INCONSISTENT_GUARD_RESULT')

    def test_approval_gate_precedes_network(self):
        for bt, usb in ((False, False), (True, False), (False, True)):
            with patch.object(runner, 'prepare', side_effect=AssertionError('must not prepare or connect')):
                with self.assertRaises(ValueError):
                    runner.run(Namespace(bt_finished=bt, usb_approved=usb))

    def test_prepare_validates_real_reviewed_artifact_offline(self):
        with patch.object(runner, 'connect_ftp', side_effect=AssertionError('no network')):
            data, manifest = runner.prepare()
        self.assertEqual(runner.digest(data), runner.EXPECTED_SHA256)
        self.assertEqual(manifest['hardware_execution'], 'NOT_PERFORMED_BY_PREPARE')

    def test_wrong_artifact_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'wrong.elf'
            path.write_bytes(b'wrong file')
            with self.assertRaises(ValueError):
                runner.prepare(path)

    def test_ftp_scope_and_permission_fail_closed(self):
        class Denied:
            def retrbinary(self, command, callback):
                raise ftplib.error_perm('550 Permission denied')
        with self.assertRaises(ftplib.error_perm):
            runner.read_remote(Denied(), runner.REMOTE_LOG, missing_ok=True)
        with self.assertRaises(ValueError):
            runner.read_remote(Denied(), '/system/sys/SceSysCore.elf')

    def test_generic_goldhen_550_requires_successful_name_listing(self):
        class GoldhenFTP:
            def __init__(self, names):
                self.names = names
            def retrbinary(self, command, callback):
                self_test.assertEqual(command, 'RETR ' + runner.REMOTE_STOP)
                raise ftplib.error_perm('550 Requested action not taken. File unavailable.')
            def retrlines(self, command, callback):
                self_test.assertEqual(command, 'NLST /data')
                for name in self.names:
                    callback(name)
        self_test = self
        self.assertIsNone(runner.read_remote(GoldhenFTP(['/data/dualsense4hen-probe.log']),
                                              runner.REMOTE_STOP, missing_ok=True))
        with self.assertRaises(ftplib.error_perm):
            runner.read_remote(GoldhenFTP(['/data/dualsense4hen.stop']), runner.REMOTE_STOP, missing_ok=True)
        with self.assertRaises(ValueError):
            runner.read_remote(GoldhenFTP(['x'] * 4097), runner.REMOTE_STOP, missing_ok=True)

    def test_collect_is_log_only_and_preserves_freshness(self):
        before = b'old transaction\n'
        for baseline, after, expected in ((before, before + transaction(), 'DRIVER_NAME_REPORTED'),
                                         (before, transaction(), 'LOG_REPLACED_OR_TRUNCATED'),
                                         (before, None, 'LOG_MISSING_AFTER_BASELINE'),
                                         (b'', None, 'WAITING_FOR_LOG')):
            class LogFTP:
                def __enter__(self):
                    return self
                def __exit__(self, *exc):
                    return False
                def retrbinary(self, command, callback):
                    self_test.assertEqual(command, 'RETR ' + runner.REMOTE_LOG)
                    if after is None:
                        raise ftplib.error_perm('550 File not found')
                    callback(after)
            self_test = self
            with tempfile.TemporaryDirectory() as folder:
                directory = Path(folder)
                runner.write_json(directory / 'manifest.json', {
                    'sha256': runner.EXPECTED_SHA256, 'baseline_sha256': runner.digest(baseline),
                    'host': 'test.invalid'})
                (directory / 'baseline.log').write_bytes(baseline)
                with patch.object(runner, 'connect_ftp', return_value=LogFTP()), \
                        patch.object(runner.socket, 'create_connection', side_effect=AssertionError('must not send ELF')):
                    result = runner.collect(directory)
                self.assertEqual(result['status'], expected)
                self.assertEqual(json.loads((directory / 'analysis.json').read_text())['status'], expected)

    def test_failed_parent_listing_never_bypasses_stop_marker_check(self):
        class FailedListing:
            def retrbinary(self, command, callback):
                raise ftplib.error_perm('550 Requested action not taken. File unavailable.')
            def retrlines(self, command, callback):
                callback('/data/dualsense4hen-probe.log')
                raise ftplib.error_temp('426 Connection closed; transfer aborted')
        with self.assertRaises(ftplib.error_temp):
            runner.read_remote(FailedListing(), runner.REMOTE_STOP, missing_ok=True)


if __name__ == '__main__':
    unittest.main()
