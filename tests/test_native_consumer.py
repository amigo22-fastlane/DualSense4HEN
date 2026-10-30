import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from native_consumer import MAX_BYTES, REMOTE_PATH, fetch, inspect_image, inventory


def elf():
    blob = bytearray(128)
    blob[:7] = b'\x7fELF\x02\x01\x01'
    struct.pack_into('<HHIQQQIHHHHHH', blob, 16,
                     2, 62, 1, 0, 64, 0, 0, 64, 56, 1, 0, 0, 0)
    struct.pack_into('<IIQQQQQQ', blob, 64, 1, 5, 0, 0, 0, 128, 128, 1)
    return blob


class FakeFTP:
    """Any unexpected FTP method/command fails; this is not console evidence."""
    def __init__(self, payload, size=None):
        self.payload = payload
        self.advertised = len(payload) if size is None else size
        self.calls = []

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def connect(self, host, port, timeout):
        assert (host, port, timeout) == ('test.invalid', 2121, 8)
        self.calls.append('connect')

    def login(self, user, password):
        assert (user, password) == ('anonymous', '')
        self.calls.append('login')

    def voidcmd(self, command):
        assert command == 'TYPE I'
        self.calls.append(command)

    def size(self, path):
        assert path == REMOTE_PATH
        self.calls.append('SIZE ' + path)
        return self.advertised

    def retrbinary(self, command, callback, blocksize):
        assert command == 'RETR ' + REMOTE_PATH
        assert blocksize == 65536
        self.calls.append(command)
        callback(self.payload[:64])
        callback(self.payload[64:])


class ConsumerTests(unittest.TestCase):
    def test_inventory_excludes_exports_and_unrelated_names(self):
        library = {'name': 'libSceHidControl', 'is_export': False,
                   'symbols': [{'name': 'sceHidControlGetDeviceInfo', 'hex_id': '123'},
                               {'name': 'sceHidControlDisconnectPort'}]}
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'consumer.json').write_text(json.dumps(
                {'modules': [{'libraries': [library]}]}), encoding='utf-8-sig')
            library['is_export'] = True
            (root / 'provider.json').write_text(json.dumps(
                {'modules': [{'libraries': [library]}]}), encoding='utf-8')
            result = inventory(root)
        self.assertEqual(result['metadata_files_scanned'], 2)
        self.assertEqual(len(result['candidates']), 1)
        self.assertEqual(result['candidates'][0]['metadata'], 'consumer.json')
        self.assertEqual(len(result['candidates'][0]['imports']), 1)
        self.assertEqual(result['candidates'][0]['imports'][0]['symbol_id_hex'], '123')

    def test_plain_elf_header_does_not_prove_abi(self):
        result = inspect_image(elf())
        self.assertEqual(result['format'], 'ELF64_X86_64')
        self.assertEqual(result['load_segments'][0]['file_size'], 128)
        self.assertEqual(result['get_device_info_abi'], 'UNKNOWN')

    def test_embedded_elf_is_not_a_plain_elf(self):
        result = inspect_image(b'container' + elf())
        self.assertEqual(result['format'], 'UNSUPPORTED_CONTAINER')
        self.assertNotIn('load_segments', result)

    def test_malformed_elf_rejected(self):
        cases = [b'\x7fELF', elf()[:100]]
        bad = elf()
        struct.pack_into('<Q', bad, 32, 2**64 - 1)
        cases.append(bad)
        bad = elf()
        struct.pack_into('<Q', bad, 64 + 32, 129)
        cases.append(bad)
        bad = elf()
        struct.pack_into('<H', bad, 56, 0xffff)
        cases.append(bad)
        for blob in cases:
            with self.subTest(length=len(blob)), self.assertRaises(ValueError):
                inspect_image(blob)

    def test_fetch_only_exact_file_and_write_local_manifest(self):
        ftp = FakeFTP(elf())
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'consumer.file'
            result = fetch('test.invalid', path, lambda: ftp)
            self.assertEqual(path.read_bytes(), elf())
            self.assertEqual(json.loads(path.with_suffix('.file.json').read_text()), result)
            self.assertFalse(result['permissions_or_configuration_changed'])
        self.assertEqual(ftp.calls, ['connect', 'login', 'TYPE I',
                                    'SIZE ' + REMOTE_PATH, 'RETR ' + REMOTE_PATH])

    def test_refuse_existing_copy_or_manifest_before_connection(self):
        for suffix in ('', '.json'):
            with tempfile.TemporaryDirectory() as folder:
                path = Path(folder) / 'consumer.file'
                existing = Path(str(path) + suffix)
                existing.write_text('preserve')
                with self.assertRaises(FileExistsError):
                    fetch('test.invalid', path, lambda: self.fail('Must not connect'))
                self.assertEqual(existing.read_text(), 'preserve')

    def test_size_gate_precedes_retr(self):
        for size in (0, -1, MAX_BYTES + 1, '128'):
            ftp = FakeFTP(elf(), size)
            with tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(ValueError):
                    fetch('test.invalid', Path(folder) / 'capture', lambda: ftp)
            self.assertNotIn('RETR ' + REMOTE_PATH, ftp.calls)

    def test_truncated_or_excess_transfer_deletes_only_own_partial(self):
        for size in (127, 129):
            ftp = FakeFTP(elf(), size)
            with tempfile.TemporaryDirectory() as folder:
                path = Path(folder) / 'capture'
                other = Path(folder) / 'preserve'
                other.write_text('keep')
                with self.assertRaises(ValueError):
                    fetch('test.invalid', path, lambda: ftp)
                self.assertFalse(path.exists())
                self.assertEqual(other.read_text(), 'keep')

    def test_unknown_container_preserved_without_unpacking(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'capture'
            result = fetch('test.invalid', path, lambda: FakeFTP(b'opaque'))
            self.assertEqual(result['format'], 'UNSUPPORTED_CONTAINER')
            self.assertEqual(path.read_bytes(), b'opaque')


if __name__ == '__main__':
    unittest.main()
