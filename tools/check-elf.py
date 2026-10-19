"""Structural validation only: no claim of console execution or GoldHEN ABI."""
import struct
import sys
from pathlib import Path

def check(path):
    data = Path(path).read_bytes()
    def unpack(fmt, offset):
        return struct.unpack_from('<' + fmt, data, offset)
    assert data[:7] == b'\x7fELF\x02\x01\x01', 'expected ELF64 little endian'
    kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = unpack('HHIQQQIHHHHHH', 16)
    assert kind == 3 and machine == 62 and version == 1, 'expected x86_64 ET_DYN'
    segments = [unpack('IIQQQQQQ', phoff + i*phsize) for i in range(phnum)]
    loads = [p for p in segments if p[0] == 1]
    assert loads and any(p[1] & 1 and p[3] <= entry < p[3]+p[6] for p in loads), 'entry not executable'
    for _, perm, off, addr, _, filesz, memsz, align in loads:
        assert filesz <= memsz and off+filesz <= len(data)
        assert align >= 0x4000 and off % align == addr % align, 'PS4 page alignment'
        assert perm & 7 != 7, 'unexpected writable executable segment'
    assert not any(p[0] == 3 for p in segments), 'interpreter dependency forbidden'
    dynamic = [p for p in segments if p[0] == 2]
    assert len(dynamic) == 1
    tags = {}
    for off in range(dynamic[0][2], dynamic[0][2]+dynamic[0][5], 16):
        tag, value = unpack('qQ', off)
        if tag == 0:
            break
        assert tag != 1, 'DT_NEEDED import: runtime must be freestanding'
        tags[tag] = value
    count = 0
    if tags.get(8, 0):
        assert tags[9] == 24 and tags[8] % 24 == 0
        addr = tags[7]
        p = next(p for p in loads if p[3] <= addr < p[3]+p[5])
        fileoff = p[2]+addr-p[3]
        for off in range(fileoff, fileoff+tags[8], 24):
            target, info, addend = unpack('QQq', off)
            assert info == 8, 'non-relative relocation / imported symbol'
            assert any(p[1] & 2 and p[3] <= target and target+8 <= p[3]+p[6] for p in loads), 'relocation outside writable ELF data'
            assert any(p[3] <= addend < p[3]+p[6] for p in loads), 'relocation points outside ELF'
            count += 1
    # Verify undefined non-local dynamic symbols cannot sneak into a call site.
    if 6 in tags:
        symaddr = tags[6]
        section_headers = [unpack('IIQQQQIIQQ', shoff+i*shsize) for i in range(shnum)]
        for section in section_headers:
            if section[1] != 11:
                continue
            assert section[9] == 24
            for off in range(section[4]+24, section[4]+section[5], 24):
                name, info, other, index, value, size = unpack('IBBHQQ', off)
                assert index != 0, 'undefined dynamic symbol'
    # Regression from console testing: an ABS image-start symbol evaluates to
    # zero under PIE and cannot identify the mapped image's base.
    section_headers = [unpack('IIQQQQIIQQ', shoff+i*shsize) for i in range(shnum)]
    found = set()
    wanted = {'__image_start','__image_rw_start','__image_end','_DYNAMIC'}
    for section in section_headers:
        if section[1] != 2:
            continue
        strings = section_headers[section[6]]
        for off in range(section[4], section[4]+section[5], 24):
            name, info, other, index, value, size = unpack('IBBHQQ', off)
            begin = strings[4]+name
            label = data[begin:data.index(b'\0', begin)].decode('ascii')
            if label in wanted:
                assert 0 < index < shnum, f'{label} must be section-relative, not ABS'
                assert other & 3 == 2, f'{label} must be hidden for pre-relocation access'
                found.add(label)
    assert found == wanted, f'missing bootstrap symbols: {wanted-found}'
    print(f'PASS {path}: ELF64 x86_64 ET_DYN, entry=0x{entry:x}, {len(loads)} LOADs, {count} relative relocations, no imports, no W+X')

for arg in sys.argv[1:]:
    check(arg)
if len(sys.argv) < 2:
    raise SystemExit('usage: python tools/check-elf.py file.elf [...]')
