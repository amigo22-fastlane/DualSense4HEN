"""Synthetic table corruption tests; no Sony fixture and no PS4 calls."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from orbis_consumer_elf import OrbisImage, native_profile, public_name_map


def fixture():
    blob = bytearray(0x600)
    blob[:7] = b'\x7fELF\x02\x01\x01'
    struct.pack_into('<HHIQQQIHHHHHH', blob, 16,
                     0xfe10, 62, 1, 0x1000, 64, 0, 0, 64, 56, 3, 0, 0, 0)
    for n, header in enumerate((
            (1, 5, 0x200, 0x1000, 0, 0x80, 0x80, 1),
            (2, 4, 0x280, 0, 0, 0x80, 0x80, 1),
            (0x61000000, 4, 0x300, 0, 0, 0x200, 0, 1))):
        struct.pack_into('<IIQQQQQQ', blob, 64 + n * 56, *header)
    blob[0x200:0x205] = b'\xe8' + struct.pack('<i', 0xb)
    blob[0x210:0x216] = b'\xff\x25' + struct.pack('<i', 0x3000 - 0x1016)
    blob[0x300:0x309] = b'\0nid#A#B\0'
    struct.pack_into('<IBBHQQ', blob, 0x340 + 24, 1, 0x12, 0, 0, 0, 0)
    struct.pack_into('<QQq', blob, 0x380, 0x3000, (1 << 32) | 7, 0)
    tags = ((0x61000035, 0), (0x61000037, 9),
            (0x61000039, 0x40), (0x6100003b, 24),
            (0x6100003f, 48), (0x61000029, 0x80),
            (0x6100002d, 24), (0, 0))
    for i, pair in enumerate(tags):
        struct.pack_into('<QQ', blob, 0x280 + i * 16, *pair)
    return blob


class OrbisConsumerTests(unittest.TestCase):
    def test_import_maps_only_known_nid(self):
        image = OrbisImage(fixture())
        self.assertEqual(image.imports({'nid': 'test_function'}),
                         [{'symbol': 'test_function', 'slot_rva': 0x3000}])
        self.assertEqual(image.imports({'other': 'other_function'}), [])
        self.assertEqual(image.identity['get_device_info_abi'], 'UNKNOWN')

    def test_branch_and_plt_resolution(self):
        image = OrbisImage(fixture())
        self.assertEqual(image.relative_branch(0x1000), 0x1010)
        self.assertEqual(image.jump_slot(0x1010), 0x3000)
        for address in (0, 0x107f, 0x1080):
            with self.assertRaises(ValueError):
                image.relative_branch(address)
        with self.assertRaises(ValueError):
            image.jump_slot(0x1000)

    def test_negative_branch_displacement(self):
        blob = fixture()
        blob[0x200:0x205] = b'\xe9' + struct.pack('<i', -5)
        self.assertEqual(OrbisImage(blob).relative_branch(0x1000), 0x1000)

    def test_undefined_function_required(self):
        for info, section in ((0x02, 0), (0x11, 0), (0x12, 1)):
            blob = fixture()
            struct.pack_into('<BBH', blob, 0x340 + 24 + 4, info, 0, section)
            self.assertEqual(OrbisImage(blob).imports({'nid': 'test'}), [])

    def test_table_bounds_and_entry_size(self):
        for tag_offset, value in ((8, 0x201), (3 * 16 + 8, 23),
                                  (4 * 16 + 8, 47), (6 * 16 + 8, 23)):
            blob = fixture()
            struct.pack_into('<Q', blob, 0x280 + tag_offset, value)
            with self.assertRaises(ValueError):
                OrbisImage(blob).imports({'nid': 'test'})

    def test_name_bounds_and_terminator(self):
        blob = fixture()
        struct.pack_into('<I', blob, 0x340 + 24, 9)
        with self.assertRaises(ValueError):
            OrbisImage(blob).imports({'nid': 'test'})
        blob = fixture()
        blob[0x308] = 65
        with self.assertRaises(ValueError):
            OrbisImage(blob).imports({'nid': 'test'})

    def test_bad_relocation_cannot_supply_call_path(self):
        for info, addend in (((20 << 32) | 7, 0), ((1 << 32) | 1, 0),
                              ((1 << 32) | 7, 4)):
            blob = fixture()
            struct.pack_into('<QQq', blob, 0x380, 0x3000, info, addend)
            with self.assertRaises(ValueError):
                OrbisImage(blob).imports({'nid': 'test'})

    def test_missing_or_duplicate_dynamic_segment(self):
        blob = fixture()
        struct.pack_into('<I', blob, 64 + 56, 1)
        with self.assertRaises(ValueError):
            OrbisImage(blob).imports({})
        blob = fixture()
        struct.pack_into('<I', blob, 64 + 2 * 56, 2)
        with self.assertRaises(ValueError):
            OrbisImage(blob).imports({})

    def test_profile_never_applies_to_a_synthetic_or_different_image(self):
        with self.assertRaises(ValueError):
            native_profile(OrbisImage(fixture()), [])
        with self.assertRaises(ValueError):
            OrbisImage(b'container' + fixture())

    def test_public_map_ignores_exports_and_unrelated_names(self):
        doc = {'modules': [{'libraries': [
            {'name': 'libSceHidControl', 'is_export': False, 'symbols': [
                {'name': 'sceHidControlGetDeviceInfo', 'encoded_id': 'nid'},
                {'name': 'sceHidControlDisconnectPort', 'encoded_id': 'other'}]},
            {'name': 'libSceBluetoothHid', 'is_export': True, 'symbols': [
                {'name': 'sceBluetoothHidGetDeviceInfo', 'encoded_id': 'export'}]}]}]}
        self.assertEqual(public_name_map(doc), {'nid': 'sceHidControlGetDeviceInfo'})


if __name__ == '__main__':
    unittest.main()
