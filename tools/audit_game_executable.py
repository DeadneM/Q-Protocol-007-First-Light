"""Read-only PE inventory and old-map audit. Matches are evidence, never remaps.

Uses only the Python standard library; does not load or execute the target.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def audit(executable, source):
    data = executable.read_bytes()
    def u16(at):
        return struct.unpack_from('<H', data, at)[0]
    def u32(at):
        return struct.unpack_from('<I', data, at)[0]
    if data[:2] != b'MZ':
        raise ValueError('Missing DOS signature')
    pe = u32(0x3c)
    if data[pe:pe + 4] != b'PE\0\0' or u16(pe + 24) != 0x20b:
        raise ValueError('Expected PE32+ executable')
    opt = pe + 24
    table = opt + u16(pe + 20)
    sections = []
    for n in range(u16(pe + 6)):
        at = table + n * 40
        sections.append(dict(name=data[at:at+8].rstrip(b'\0').decode('ascii'),
                             virtual_size=u32(at+8), rva=u32(at+12),
                             raw_size=u32(at+16), offset=u32(at+20),
                             characteristics=u32(at+36)))
    def locate(rva, size=32):
        for s in sections:
            relative = rva - s['rva']
            if 0 <= relative < max(s['virtual_size'], s['raw_size']):
                count = min(size, max(0, s['raw_size'] - relative))
                at = s['offset'] + relative
                return s['name'], data[at:at+count]
        return None, b''
    src = source.read_text(encoding='utf-8')
    mappings = {}
    for name, literal in re.findall(r'constexpr std::uintptr_t (k\w+Rva)\s*=\s*(0x[0-9A-Fa-f]+)', src):
        rva = int(literal, 16)
        section, sample = locate(rva)
        mappings[name] = dict(rva=f'0x{rva:08X}', section=section, bytes=sample.hex(' '), status='UNVALIDATED')
    preimages = {}
    for name, body in re.findall(r'constexpr BYTE (k\w+Preimage)\[\d+\]\s*=\s*\{([^}]+)\}', src):
        pattern = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', body))
        site = name.replace('Preimage', 'Rva')
        _, current = locate(int(mappings[site]['rva'], 16), len(pattern))
        matches = []
        for s in sections:
            if not s['characteristics'] & 0x20000000:
                continue
            start, end = s['offset'], min(len(data), s['offset'] + s['raw_size'])
            cursor = start
            while (cursor := data.find(pattern, cursor, end)) >= 0:
                matches.append(f"0x{s['rva'] + cursor - start:08X}")
                cursor += 1
        preimages[name] = dict(pattern=pattern.hex(' '), old_site_matches=current == pattern,
                               executable_section_candidates=matches,
                               warning='Byte matches alone do not establish native function identity or ABI.')
    return dict(path=str(executable.resolve()), length=len(data), sha256=hashlib.sha256(data).hexdigest(),
                machine=f'0x{u16(pe+4):04X}', timestamp=f'0x{u32(pe+8):08X}',
                image_base=f'0x{struct.unpack_from("<Q", data, opt+24)[0]:X}',
                size_of_image=f'0x{u32(opt+56):08X}', sections=sections,
                source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                old_mappings=mappings, preimages=preimages,
                verdict='INVENTORY ONLY: no RVA or compatibility approval is implied.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1] / 'src/QProtocol.cpp')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.executable, args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ('length', 'sha256', 'timestamp', 'size_of_image', 'preimages')}, indent=2))
