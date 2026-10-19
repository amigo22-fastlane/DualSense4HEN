import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from app_metadata import check_ids, sfo_fields


class AppMetadataTests(unittest.TestCase):
    def test_good_id(self):
        check_ids('IV0000-DSHN00001_00-DS4HENUSBDIAG001', 'DSHN00001')

    def test_rejected_first_package_regression(self):
        with self.assertRaises(ValueError):
            check_ids('IV0000-D4HN00001_00-DS4HENUSBDIAG001', 'D4HN00001')

    def test_mismatch(self):
        with self.assertRaises(ValueError):
            check_ids('IV0000-DSHN00001_00-DS4HENUSBDIAG001', 'DSHN00002')

    def test_truncated_id(self):
        with self.assertRaises(ValueError):
            check_ids('IV0000-DSHN00001_00-DS4HENUSBDIAG00', 'DSHN00001')

    def test_sfo_layout(self):
        key, value = b'TITLE_ID\0', b'DSHN00001\0'
        data = struct.pack('<5I', 0x46535000, 0x101, 36, 36 + len(key), 1)
        data += struct.pack('<HHIII', 0, 0x204, len(value), len(value), 0) + key + value
        self.assertEqual(sfo_fields(data)['TITLE_ID'], 'DSHN00001')
        with self.assertRaises(ValueError):
            sfo_fields(data[:-1])

    def test_bad_header(self):
        with self.assertRaises(ValueError):
            sfo_fields(b'not an SFO')


if __name__ == '__main__':
    unittest.main()
