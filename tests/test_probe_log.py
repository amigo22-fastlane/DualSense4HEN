"""Regression checks for stale cumulative hardware results."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from probe_log import appended_log, usb_outcome


class ProbeLogTests(unittest.TestCase):
    def test_no_new_run(self):
        old = b'USB probe revision=2 begin;\nUSB summary good=9\nprobe exit=0\n'
        self.assertEqual(appended_log(old, old), b'')
        self.assertIn('unconfirmed', usb_outcome(appended_log(old, old)))

    def test_old_success_new_pending(self):
        old = b'probe exit=0\n'
        new = b'USB probe revision=2 begin;\n'
        self.assertIn('incomplete', usb_outcome(appended_log(old, old + new)))

    def test_new_failure(self):
        log = b'USB probe revision=2 begin;\nsceUsbdGetDeviceList count=0\nprobe exit=-22\n'
        self.assertIn('without full capture success', usb_outcome(log))

    def test_complete(self):
        log = b'USB probe revision=2 begin;\nUSB summary good=1\nprobe exit=0\n'
        self.assertIn('reports completion', usb_outcome(log))

    def test_replaced_log(self):
        with self.assertRaises(ValueError):
            appended_log(b'old log', b'new log')

    def test_revision_three(self):
        log = b'USB probe revision=3 begin;\nUSB summary good=1\nprobe exit=0\n'
        self.assertIn('reports completion', usb_outcome(log))

    def test_last_run_wins(self):
        log = (b'USB probe revision=2 begin;\nUSB summary good=1\nprobe exit=0\n'
               b'USB probe revision=2 begin;\nprobe exit=-22\n')
        self.assertIn('without full capture success', usb_outcome(log))


if __name__ == '__main__':
    unittest.main()
