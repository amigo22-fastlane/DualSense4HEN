"""Generate the project's own 512x512 RGBA diagnostic icon, stdlib only."""
import struct
import sys
import zlib
from pathlib import Path

glyphs = {
    'D': ['1110','1001','1001','1001','1001','1001','1110'],
    '4': ['1001','1001','1001','1111','0001','0001','0001'],
    'H': ['1001','1001','1001','1111','1001','1001','1001'],
    'U': ['1001','1001','1001','1001','1001','1001','0110'],
    'S': ['0111','1000','1000','0110','0001','0001','1110'],
    'B': ['1110','1001','1001','1110','1001','1001','1110'],
}
rows = [bytearray([17, 25, 42, 255] * 512) for _ in range(512)]
for word, top in [('D4H', 90), ('USB', 285)]:
    scale = 20
    for index, letter in enumerate(word):
        for y, row in enumerate(glyphs[letter]):
            for x, bit in enumerate(row):
                if bit == '1':
                    for yy in range(top + y * scale, top + (y + 1) * scale):
                        for xx in range(116 + index * 100 + x * scale, 116 + index * 100 + (x + 1) * scale):
                            rows[yy][xx * 4:xx * 4 + 4] = bytes([93, 195, 255, 255])

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

png = b'\x89PNG\r\n\x1a\n'
png += chunk(b'IHDR', struct.pack('>IIBBBBB', 512, 512, 8, 6, 0, 0, 0))
png += chunk(b'IDAT', zlib.compress(b''.join(b'\0' + row for row in rows)))
png += chunk(b'IEND', b'')
Path(sys.argv[1]).write_bytes(png)
