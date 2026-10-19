"""Print derived error-code mappings from the owner's approved local BT dump.

No console connection, modification, extraction or disassembly output.
Offsets belong solely to the documented SHA-256 profile, not kernel memory.
Input must remain local and ignored by Git.
"""
import argparse
import hashlib
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('text_segment', type=Path)
args = parser.parse_args()
code = args.text_segment.read_bytes()
expected = '48a4c7f14870555d0f9843f246b705cf821167e157e37d7bb53a0ef78090afc2'
if len(code) != 16384 or hashlib.sha256(code).hexdigest() != expected:
    raise SystemExit('Unrecognized module profile; refusing offset interpretation.')

# RegisterDevice's relative jump table. Its shared unlock path retains the
# preloaded 0x81540002 value; other cases first assign a different status.
table = 0x1ab0
for index in range(12):
    target = table + struct.unpack_from('<i', code, table + index * 4)[0]
    if target == 0xb44:
        status = 0x81540002
    elif 0 <= target <= len(code) - 5 and code[target] == 0xbb:
        status = struct.unpack_from('<I', code, target + 1)[0]
    else:
        raise SystemExit('Unexpected error mapping branch; refusing interpretation.')
    print(f'0x{0x802f3701 + index:08x} -> 0x{status:08x}')
