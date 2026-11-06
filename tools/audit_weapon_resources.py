"""Read-only First Light RPKG index/catalog audit; never extracts or modifies assets.

Format reference: glacier-modding/Bond-Hashes/scripts/extract_hashes.py.
This supported installation uses 20-byte resource metadata, not the 24-byte
Hitman layout. Validate the entire metadata walk rather than guessing by name.
"""
import argparse
import configparser
import hashlib
import json
import struct
import subprocess
from pathlib import Path

REFERENCE_COMMIT = '94767af01ce524975acbb254e4cd98e28a04d061'
REFERENCE_BLOB = 'ab3a42fe3635dc703d5dc634f05229b6032c0af7'


def audit(runtime, hashes, config):
    raw = hashes.read_bytes()
    assert hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest() == REFERENCE_BLOB
    paths = {entry['hash']: entry['path'] for entry in json.loads(raw) if entry.get('path')}
    ini = configparser.ConfigParser()
    ini.optionxform = str
    ini.read_string(config.read_text(encoding='utf-8-sig'))
    wanted = {int(rid, 16): alias for alias, rid in ini['WeaponCatalog'].items()}
    assert len(wanted) == len(ini['WeaponCatalog']), 'Duplicate catalog RID'
    found = {rid: [] for rid in wanted}
    deleted = {rid: [] for rid in wanted}
    packages = []
    for file in sorted(runtime.glob('*.rpkg')):
        file_size = file.stat().st_size
        with file.open('rb') as stream:
            magic = stream.read(4)
            assert magic in (b'2KPR', b'GKPR'), file.name
            base = 13 if magic == b'2KPR' else 4
            stream.seek(base)
            count, headers_size, metadata_size = struct.unpack('<III', stream.read(12))
            assert count and headers_size == count*20 and headers_size+metadata_size <= file_size
            start = base+12
            stream.seek(start)
            patch_count = struct.unpack('<I', stream.read(4))[0]
            candidates = [start]
            if patch_count < 10000000 and start+4+patch_count*8+headers_size+metadata_size <= file_size:
                candidates.append(start+4+patch_count*8)
            valid = []
            for candidate in candidates:
                stream.seek(candidate)
                _, first_data, _ = struct.unpack('<QQI', stream.read(20))
                if first_data == candidate+headers_size+metadata_size:
                    valid.append(candidate)
            assert len(valid) == 1, (file.name, 'unsupported/ambiguous index layout')
            table_start = valid[0]
            if table_start != start:
                stream.seek(start+4)
                for _ in range(patch_count):
                    rid = struct.unpack('<Q', stream.read(8))[0]
                    if rid in deleted:
                        deleted[rid].append(file.name)
            stream.seek(table_start)
            headers = stream.read(headers_size)
            metadata = stream.read(metadata_size)
            assert len(metadata) == metadata_size
            cursor = 0
            for i in range(count):
                assert cursor+20 <= metadata_size, file.name
                kind = metadata[cursor:cursor+4][::-1]
                depends, final_size = struct.unpack_from('<II', metadata, cursor+4)
                assert all(48 <= b <= 90 for b in kind) and depends <= metadata_size-cursor-20, file.name
                rid, offset, flags = struct.unpack_from('<QQI', headers, i*20)
                size = flags & 0x3fffffff or final_size
                assert table_start+headers_size+metadata_size <= offset <= file_size
                assert size <= file_size-offset
                if rid in found:
                    found[rid].append({'package': file.name, 'type': kind.decode(), 'offset': offset, 'storedSize': size})
                cursor += 20+depends
            assert cursor == metadata_size, file.name
            packages.append({'name': file.name, 'resources': count, 'metadataEntrySize': 20,
                             'indexSha256': hashlib.sha256(headers+metadata).hexdigest()})
    assert packages, 'No packages inspected'
    entries = []
    for rid, alias in wanted.items():
        text = f'{rid:016X}'
        path = paths.get(text)
        computed = '01'+hashlib.md5(path.lower().encode()).hexdigest().upper()[2:16] if path else None
        entries.append({'alias': alias, 'rid': text, 'path': path, 'pathHashMatches': computed == text,
                        'occurrences': found[rid], 'patchDeletions': deleted[rid],
                        'presentAsTEMP': any(x['type'] == 'TEMP' for x in found[rid])})
    return {'reference': {'repository': 'glacier-modding/Bond-Hashes', 'commit': REFERENCE_COMMIT,
                          'TEMPBlob': REFERENCE_BLOB, 'sha256': hashlib.sha256(raw).hexdigest()},
            'catalogSha256': hashlib.sha256(config.read_bytes()).hexdigest(),
            'packages': packages, 'entries': entries,
            'limits': 'Resource identity/presence only, not acquisition or gameplay validation. Unknown package layouts fail closed; load order is not inferred.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--hashes', type=Path, required=True)
    parser.add_argument('--config', type=Path, default=Path('config/QProtocol.ini'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.runtime, args.hashes, args.config)
    report['source'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    entries = report['entries']
    print(json.dumps({'entries': len(entries), 'presentAsTEMP': sum(x['presentAsTEMP'] for x in entries),
                      'pathHashesMatch': sum(x['pathHashMatches'] for x in entries),
                      'patchDeletions': sum(bool(x['patchDeletions']) for x in entries), 'output': str(args.output)}))
