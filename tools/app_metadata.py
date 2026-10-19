"""PS4 application metadata checks; separate from cryptographic PKG validation.

Format corroborated with maxton/LibOrbisPkg GP4/Gp4Validator.cs.
No code from that implementation is copied.
"""
import re
import struct


def check_ids(content, title):
    if not re.fullmatch(r'[A-Z]{2}[0-9]{4}-[A-Z]{4}[0-9]{5}_00-[A-Z0-9]{16}', content):
        raise ValueError('Invalid application Content ID format')
    if not re.fullmatch(r'[A-Z]{4}[0-9]{5}', title) or content[7:16] != title:
        raise ValueError('TITLE_ID does not match Content ID')


def sfo_fields(data):
    if len(data) < 20 or data[:4] != b'\0PSF':
        raise ValueError('Invalid SFO header')
    _, version, keys, values, count = struct.unpack_from('<5I', data)
    if version != 0x101 or count > 256 or not 20 + 16 * count <= keys <= values <= len(data):
        raise ValueError('Invalid SFO layout')
    result = {}
    for index in range(count):
        key, fmt, length, maximum, offset = struct.unpack_from('<HHIII', data, 20 + 16 * index)
        pos = keys + key
        end = data.find(b'\0', pos, values)
        if pos < keys or pos >= values or end < pos or length > maximum or values + offset + maximum > len(data):
            raise ValueError('Invalid SFO entry bounds')
        name = data[pos:end].decode('ascii')
        if name in result:
            raise ValueError('Duplicate SFO key')
        payload = data[values + offset:values + offset + length]
        if fmt == 0x204:
            if not payload.endswith(b'\0'):
                raise ValueError('Unterminated SFO string')
            result[name] = payload[:-1].decode('utf-8')
        elif fmt == 0x404 and length == 4:
            result[name] = struct.unpack('<I', payload)[0]
    return result
