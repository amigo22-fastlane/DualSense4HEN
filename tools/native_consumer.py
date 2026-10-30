"""Offline import inventory and bounded, opt-in FTP copy of one native consumer.

Does not execute a PS4 payload, query HID, decrypt SELF, or read process memory.
The fetch subcommand requires separate owner approval before use.
"""
import argparse
from datetime import datetime, timezone
import ftplib
import hashlib
import json
from pathlib import Path
import struct

REMOTE_PATH = '/system/sys/SceSysCore.elf'
MAX_BYTES = 16 * 1024 * 1024
TARGETS = {
    'libSceHidControl': {'sceHidControlGetDeviceInfo',
                         'sceHidControlGetDeviceName', 'sceHidControlGetBatteryState'},
    'libSceBluetoothHid': {'sceBluetoothHidGetDeviceInfo'},
}


def inventory(root):
    """Only public metadata; imports do not establish a consumer's ABI."""
    candidates = []
    paths = sorted(root.rglob('*.json'))
    for path in paths:
        raw = path.read_bytes()
        obj = json.loads(raw.decode('utf-8-sig'))
        found = []
        for module in obj.get('modules', []):
            for library in module.get('libraries', []):
                names = TARGETS.get(library.get('name'), set())
                if library.get('is_export') is not False:
                    continue
                for symbol in library.get('symbols', []):
                    if symbol.get('name') in names:
                        found.append({'library': library['name'],
                                      'symbol': symbol['name'],
                                      'symbol_id_hex': symbol.get('hex_id')})
        if found:
            candidates.append({'metadata': path.relative_to(root).as_posix(),
                               'metadata_sha256': hashlib.sha256(raw).hexdigest(),
                               'imports': found})
    return {'metadata_files_scanned': len(paths), 'candidates': candidates,
            'abi': 'UNKNOWN: import names do not give output size or selector semantics'}


def inspect_image(blob):
    """Parse only a top-level ELF64 header. Never treat embedded ELF as decrypted."""
    result = {'size': len(blob), 'sha256': hashlib.sha256(blob).hexdigest(),
              'format': 'UNSUPPORTED_CONTAINER', 'get_device_info_abi': 'UNKNOWN'}
    if len(blob) > MAX_BYTES:
        raise ValueError('Image exceeds fixed 16 MiB scope')
    if not blob.startswith(b'\x7fELF'):
        return result  # Could be SELF; no assumption, unpacking or execution.
    if len(blob) < 64 or blob[4:7] != b'\x02\x01\x01':
        raise ValueError('Not a complete ELF64 little-endian v1 header')
    fields = struct.unpack_from('<HHIQQQIHHHHHH', blob, 16)
    kind, machine, version, _, phoff, _, _, ehsize, phsize, phcount, _, _, _ = fields
    if machine != 62 or version != 1 or ehsize != 64:
        raise ValueError('Unsupported ELF header')
    if phsize != 56 or not 1 <= phcount <= 256 or phoff < 64:
        raise ValueError('Invalid program-header layout')
    if phoff + phsize * phcount > len(blob):
        raise ValueError('Truncated program-header table')
    loads = []
    for i in range(phcount):
        ptype, flags, offset, _, _, filesz, memsz, _ = struct.unpack_from(
            '<IIQQQQQQ', blob, phoff + i * phsize)
        if filesz and (offset > len(blob) or filesz > len(blob) - offset):
            raise ValueError('Segment exceeds file bounds')
        if ptype == 1:
            if filesz > memsz:
                raise ValueError('LOAD filesz exceeds memsz')
            loads.append({'file_offset': offset, 'file_size': filesz, 'flags': flags})
    result.update(format='ELF64_X86_64', elf_type=f'0x{kind:04x}', load_segments=loads)
    return result


def fetch(host, destination, ftp_factory=ftplib.FTP):
    """Exact-file RETR only. No scan, remote write, chmod or fallback access."""
    destination = Path(destination)
    manifest = destination.with_suffix(destination.suffix + '.json')
    if destination.exists() or manifest.exists():
        raise FileExistsError('Refusing to overwrite a local capture or manifest')
    # Parent must already exist: caller chooses a local, ignored evidence directory.
    created = False
    try:
        with ftp_factory() as ftp:
            ftp.connect(host, 2121, timeout=8)
            ftp.login('anonymous', '')
            ftp.voidcmd('TYPE I')
            size = ftp.size(REMOTE_PATH)
            if type(size) is not int or not 0 < size <= MAX_BYTES:
                raise ValueError('SIZE unavailable, empty or exceeds fixed 16 MiB scope')
            count = 0
            with destination.open('xb') as output:
                created = True

                def receive(block):
                    nonlocal count
                    if len(block) > size - count:
                        raise ValueError('RETR exceeds advertised SIZE; stopping')
                    output.write(block)
                    count += len(block)

                ftp.retrbinary('RETR ' + REMOTE_PATH, receive, blocksize=65536)
            if count != size:
                raise ValueError('Incomplete RETR; stopping')
        evidence = inspect_image(destination.read_bytes())
        evidence.update(remote_path=REMOTE_PATH, port=2121,
                        captured_at_utc=datetime.now(timezone.utc).isoformat(),
                        source='on-disk file, not live process memory',
                        firmware='NOT_MEASURED_BY_THIS_TOOL',
                        permissions_or_configuration_changed=False)
        with manifest.open('x', encoding='utf-8') as output:
            json.dump(evidence, output, indent=2)
            output.write('\n')
        return evidence
    except Exception:
        if created:
            destination.unlink(missing_ok=True)  # Only this run's local partial copy.
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest='command', required=True)
    inv = subs.add_parser('inventory')
    inv.add_argument('root', type=Path)
    image = subs.add_parser('inspect')
    image.add_argument('image', type=Path)
    copy = subs.add_parser('fetch', help='Requires prior explicit owner approval')
    copy.add_argument('host')
    copy.add_argument('destination', type=Path)
    args = parser.parse_args()
    if args.command == 'inventory':
        evidence = inventory(args.root)
    elif args.command == 'inspect':
        if args.image.stat().st_size > MAX_BYTES:
            parser.error('Image exceeds scope')
        evidence = inspect_image(args.image.read_bytes())
    else:
        evidence = fetch(args.host, args.destination)
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
