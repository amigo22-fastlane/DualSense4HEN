"""Offline evidence from an owner-approved, plain Orbis ELF file.

No network, execution, decryption or disassembly output. Dynamic tag meanings:
OpenOrbis scripts/dynamic_entries.py and scripts/symbol_entries.py.
Numeric call-site facts apply ONLY to the full SHA256 profile below.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from native_consumer import inspect_image, TARGETS, MAX_BYTES

NATIVE_SHA = '968fd8d7ac0f65d3949fed95e8d113e57ee77108bf349aad545ea639cf893b69'


class OrbisImage:
    def __init__(self, blob):
        self.identity = inspect_image(blob)
        if self.identity['format'] != 'ELF64_X86_64':
            raise ValueError('Only a top-level plain ELF64 is supported')
        self.blob = blob
        start = struct.unpack_from('<Q', blob, 32)[0]
        size, count = struct.unpack_from('<HH', blob, 54)
        self.headers = [struct.unpack_from('<IIQQQQQQ', blob, start + i * size)
                        for i in range(count)]

    def only_header(self, kind):
        found = [p for p in self.headers if p[0] == kind]
        if len(found) != 1:
            raise ValueError('Expected one segment of the requested type')
        return found[0]

    def file_slice(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.blob) or size > len(self.blob) - offset:
            raise ValueError('File range outside image')
        return self.blob[offset:offset + size]

    def virtual_slice(self, address, size):
        ranges = [p for p in self.headers if p[0] in (1, 0x61000010)
                  and address >= p[3] and address - p[3] <= p[5]
                  and size <= p[5] - (address - p[3])]
        if len(ranges) != 1:
            raise ValueError('Virtual range is not uniquely file-backed')
        p = ranges[0]
        return self.file_slice(p[2] + address - p[3], size)

    def relative_branch(self, address):
        instruction = self.virtual_slice(address, 5)
        if instruction[0] not in (0xe8, 0xe9):
            raise ValueError('Expected a direct near call/jump at evidence site')
        return address + 5 + struct.unpack_from('<i', instruction, 1)[0]

    def jump_slot(self, address):
        instruction = self.virtual_slice(address, 6)
        if instruction[:2] != b'\xff\x25':
            raise ValueError('Expected RIP-relative indirect jump at PLT evidence site')
        return address + 6 + struct.unpack_from('<i', instruction, 2)[0]

    def imports(self, name_map):
        dynamic = self.only_header(2)
        link = self.only_header(0x61000000)
        raw_dynamic = self.file_slice(dynamic[2], dynamic[5])
        if len(raw_dynamic) % 16:
            raise ValueError('Misaligned dynamic table')
        tags = {}
        for pos in range(0, len(raw_dynamic), 16):
            tag, value = struct.unpack_from('<QQ', raw_dynamic, pos)
            if not tag:
                break
            if tag in (0x61000035, 0x61000037, 0x61000039, 0x6100003b,
                       0x6100003f, 0x61000029, 0x6100002d):
                if tag in tags:
                    raise ValueError('Duplicate required dynamic tag')
                tags[tag] = value
        else:
            raise ValueError('Missing dynamic terminator')

        def table(offset_tag, size_tag):
            if offset_tag not in tags or size_tag not in tags:
                raise ValueError('Missing SCE dynamic table')
            offset, size = tags[offset_tag], tags[size_tag]
            if offset > link[5] or size > link[5] - offset:
                raise ValueError('SCE table outside DYNLIBDATA')
            return self.file_slice(link[2] + offset, size)

        strings = table(0x61000035, 0x61000037)
        symbols = table(0x61000039, 0x6100003f)
        jumps = table(0x61000029, 0x6100002d)
        if tags.get(0x6100003b) != 24 or len(symbols) % 24 or len(jumps) % 24:
            raise ValueError('Unsupported symbol/relocation entry size')
        imports = {}
        for pos in range(0, len(symbols), 24):
            no, info, _, section, _, _ = struct.unpack_from('<IBBHQQ', symbols, pos)
            if section != 0 or info >> 4 == 0 or info & 15 != 2:
                continue
            if no >= len(strings):
                raise ValueError('Symbol name outside string table')
            end = strings.find(b'\0', no)
            if end < 0:
                raise ValueError('Unterminated symbol name')
            encoded = strings[no:end].decode('ascii')
            nid = encoded.split('#')[0]
            if nid in name_map:
                imports[pos // 24] = name_map[nid]
        matched = []
        for pos in range(0, len(jumps), 24):
            address, info, addend = struct.unpack_from('<QQq', jumps, pos)
            index = info >> 32
            if index >= len(symbols) // 24:
                raise ValueError('Relocation symbol index outside table')
            if index in imports:
                if info & 0xffffffff != 7 or addend != 0:
                    raise ValueError('Unexpected relocation for function import')
                matched.append({'symbol': imports[index], 'slot_rva': address})
        return matched


def public_name_map(document):
    result = {}
    for module in document.get('modules', []):
        for library in module.get('libraries', []):
            allowed = TARGETS.get(library.get('name'), set())
            if library.get('is_export') is not False:
                continue
            for symbol in library.get('symbols', []):
                if symbol.get('name') in allowed:
                    nid = symbol['encoded_id']
                    if nid in result and result[nid] != symbol['name']:
                        raise ValueError('Ambiguous public NID mapping')
                    result[nid] = symbol['name']
    return result


def native_profile(image, imports):
    if image.identity['sha256'] != NATIVE_SHA:
        raise ValueError('Unknown full image; refusing call-site profile')
    # All locations below were established offline in the approved local file.
    # They are image RVAs, never offsets to modify or read from a live process.
    paths = (
        ('sceHidControlGetDeviceInfo', (0x70824, 0x5380f, 0x5b61f),
         (0x537f0, 0x5b5f0, 0x91ec0)),
        ('sceBluetoothHidGetDeviceInfo', (0x7181c, 0x5383f, 0x5b68f),
         (0x53820, 0x5b660, 0x91e40)),
    )
    slots = {item['symbol']: item['slot_rva'] for item in imports}
    checked = []
    for name, sites, targets in paths:
        for site, target in zip(sites, targets):
            if image.relative_branch(site) != target:
                raise ValueError('Unexpected direct branch in native profile')
        if image.jump_slot(targets[-1]) != slots.get(name):
            raise ValueError('PLT does not resolve to the named imported slot')
        checked.append({'symbol': name,
                        'branch_sites_rva': [f'0x{x:x}' for x in sites],
                        'targets_rva': [f'0x{x:x}' for x in targets],
                        'slot_rva': f'0x{slots[name]:x}'})
    return {'sha256': NATIVE_SHA, 'verified_static_paths': checked,
            'evidence_kind': 'STATIC_CALL_SITES_NOT_A_RUNTIME_TRACE',
            'get_device_info_abi': 'PARTIAL_NO_QUERY',
            'selector': 'native device identifier narrowed to low 32 bits',
            'hid_output_size': 'UNKNOWN', 'bt_output_size': 'UNKNOWN',
            'hid_minimum_read_extent_bytes': 37, 'bt_minimum_read_extent_bytes': 12,
            'device_name_zeroed_buffer_bytes': 514,
            'l2cap_control': 'UNKNOWN', 'l2cap_interrupt': 'UNKNOWN',
            'hid_report_reception': 'UNKNOWN'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('public_metadata', type=Path)
    parser.add_argument('--native-profile', action='store_true')
    args = parser.parse_args()
    if args.image.stat().st_size > MAX_BYTES:
        parser.error('Image exceeds approved 16 MiB scope')
    image = OrbisImage(args.image.read_bytes())
    document = json.loads(args.public_metadata.read_text(encoding='utf-8-sig'))
    imports = image.imports(public_name_map(document))
    result = {'image': image.identity, 'matched_imports': imports}
    if args.native_profile:
        result['native_profile'] = native_profile(image, imports)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
