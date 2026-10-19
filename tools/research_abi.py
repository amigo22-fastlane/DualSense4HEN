"""Numeric ABI evidence from owner-approved local images; emits no Sony code.
Never imports this file to issue a console operation or ioctl.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

USBD_SHA = 'dc8aa9f70baa6f6e32d1ea938cdd51761f6601aa4c7e63ff11f6509b848b5dff'
HID_SHA = '90a1f20a98572dd0838eba12c8ee2b6560c4de749570e9f475f9c2a056312c17'


def mov_esi_value(code, offset):
    if offset < 0 or offset + 5 > len(code) or code[offset] != 0xbe:
        raise ValueError('Unexpected instruction shape')
    return struct.unpack_from('<I', code, offset + 1)[0]


def inspect(kind, code):
    if kind not in ('usbd', 'hidcontrol'):
        raise ValueError('Unknown audit kind')
    expected, length = (USBD_SHA, 65536) if kind == 'usbd' else (HID_SHA, 16384)
    if len(code) != length or hashlib.sha256(code).hexdigest() != expected:
        raise ValueError('Unknown library image; refusing profile-relative interpretation')
    sites = ({'DRIVER_ACTIVE': 0x92be, 'DRIVER_DETACH': 0x904e,
              'FS_INIT': 0x8cb5, 'FS_OPEN': 0x9b3b,
              'GET_IFACE_DRIVER': 0x8648, 'DO_REQUEST': 0x90d5,
              'SET_POWER_MODE': 0x9628} if kind == 'usbd' else
             {'GET_DEVICE_INFO': 0x37b, 'GET_DEVICE_NAME': 0x53b,
              'GET_BATTERY_STATE': 0x4d2})
    return {'kind': kind, 'sha256': expected,
            'requests': {k: f'0x{mov_esi_value(code, v):08x}' for k, v in sites.items()},
            'kernel_semantics': 'NOT_PROVEN_BY_WRAPPER_ALONE',
            'hid_output_size': 'UNKNOWN' if kind == 'hidcontrol' else 'not applicable'}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('kind', choices=['usbd', 'hidcontrol'])
    p.add_argument('image', type=Path)
    args = p.parse_args()
    print(json.dumps(inspect(args.kind, args.image.read_bytes()), indent=2))
