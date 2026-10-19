"""Synthetic protocol vectors, NOT captured controller reports. No dependencies.
CRC oracle is Python zlib, independent of the C CRC implementation.
"""
from pathlib import Path
import zlib

dest = Path(__file__).resolve().parents[1] / 'tests' / 'fixtures'
body = bytearray(63)
body[:10] = bytes([0, 255, 64, 192, 17, 254, 0x42, 0x21, 0x30, 0x07])
body[15:21] = bytes.fromhex('00 80 ff 7f ff ff')
body[21:27] = bytes.fromhex('00 00 01 00 00 c0')
body[27:31] = bytes.fromhex('78 56 34 12')
# Contact 7 at (1919,1079), contact 8 inactive (garbage coordinates ignored).
body[32:40] = bytes.fromhex('07 7f 77 43 88 ff ff ff')
vectors = {'usb-controls.hex': bytes([1]) + body,
           'bt-simple.hex': bytes.fromhex('01 00 ff 40 c0 21 30 03 11 fe')}
bt = bytes([0x31, 0xa0]) + body + bytes(9)
assert len(bt) == 74
vectors['bt-extended.hex'] = bt + zlib.crc32(b'\xa1' + bt).to_bytes(4, 'little')
neutral = bytearray(64)
neutral[:5] = bytes([1,128,128,128,128])
neutral[8] = 8
neutral[33] = neutral[37] = 0x80
vectors['usb-neutral.hex'] = neutral
for name, data in vectors.items():
    (dest / name).write_text(' '.join(f'{b:02x}' for b in data) + '\n', encoding='ascii')
