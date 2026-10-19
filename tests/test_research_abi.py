import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from research_abi import inspect, mov_esi_value


class ResearchAbiTests(unittest.TestCase):
    def test_numeric_decoder_synthetic_instruction(self):
        self.assertEqual(mov_esi_value(bytes.fromhex('be 7c 55 04 80'), 0), 0x8004557c)

    def test_truncated_or_wrong_opcode(self):
        for data, offset in ((b'', 0), (b'\xbe\x00', 0), (b'\x90'*5, 0), (b'\xbe'*5, -1)):
            with self.assertRaises(ValueError):
                mov_esi_value(data, offset)

    def test_cannot_interpret_another_module(self):
        for kind, size in (('usbd', 65536), ('hidcontrol', 16384)):
            with self.assertRaises(ValueError):
                inspect(kind, b'\x00'*size)


if __name__ == '__main__':
    unittest.main()
