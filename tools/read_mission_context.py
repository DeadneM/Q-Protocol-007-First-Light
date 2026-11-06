"""Bounded, read-only external snapshot. Never injects, suspends or writes memory.

Run while the user leaves the game in the named screen. Output is private and
must remain outside Git. A snapshot is evidence, not gameplay validation.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
from pathlib import Path
import struct
import time

EXPECTED_EXE = '6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1'
ROOT_RVA = 0x6452BF8
TABLE_RVA = 0x5CF7688
LIST_RVA = 0x671FFC0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--label', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise SystemExit('Private snapshots must be written outside the repository')
    if c.sizeof(c.c_void_p) != 8:
        raise SystemExit('64-bit Python required')
    k = c.WinDLL('kernel32', use_last_error=True)
    nt = c.WinDLL('ntdll')
    invalid = c.c_void_p(-1).value

    class Module(c.Structure):
        _fields_ = [('dwSize', w.DWORD), ('th32ModuleID', w.DWORD),
                    ('th32ProcessID', w.DWORD), ('GlblcntUsage', w.DWORD),
                    ('ProccntUsage', w.DWORD), ('modBaseAddr', c.c_void_p),
                    ('modBaseSize', w.DWORD), ('hModule', c.c_void_p),
                    ('szModule', w.WCHAR * 256), ('szExePath', w.WCHAR * 260)]

    class Process(c.Structure):
        _fields_ = [('dwSize', w.DWORD), ('cntUsage', w.DWORD),
                    ('th32ProcessID', w.DWORD), ('th32DefaultHeapID', c.c_size_t),
                    ('th32ModuleID', w.DWORD), ('cntThreads', w.DWORD),
                    ('th32ParentProcessID', w.DWORD), ('pcPriClassBase', w.LONG),
                    ('dwFlags', w.DWORD), ('szExeFile', w.WCHAR * 260)]

    class Thread(c.Structure):
        _fields_ = [('dwSize', w.DWORD), ('cntUsage', w.DWORD),
                    ('th32ThreadID', w.DWORD), ('th32OwnerProcessID', w.DWORD),
                    ('tpBasePri', w.LONG), ('tpDeltaPri', w.LONG), ('dwFlags', w.DWORD)]

    class BasicThread(c.Structure):
        _fields_ = [('ExitStatus', w.LONG), ('TebBaseAddress', c.c_void_p),
                    ('UniqueProcess', c.c_void_p), ('UniqueThread', c.c_void_p),
                    ('AffinityMask', c.c_size_t), ('Priority', w.LONG),
                    ('BasePriority', w.LONG)]

    k.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
    k.CreateToolhelp32Snapshot.restype = w.HANDLE
    k.CloseHandle.argtypes = [w.HANDLE]
    k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    k.OpenProcess.restype = w.HANDLE
    k.OpenThread.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    k.OpenThread.restype = w.HANDLE
    k.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
    k.ReadProcessMemory.restype = w.BOOL
    nt.NtQueryInformationThread.argtypes = [w.HANDLE, w.ULONG, c.c_void_p, w.ULONG, c.c_void_p]
    nt.NtQueryInformationThread.restype = w.LONG
    for prefix, cls in [('Process', Process), ('Module', Module), ('Thread', Thread)]:
        for suffix in ['First', 'Next']:
            name = prefix + '32' + suffix + ('' if prefix == 'Thread' else 'W')
            getattr(k, name).argtypes = [w.HANDLE, c.POINTER(cls)]
            getattr(k, name).restype = w.BOOL

    def entries(flag, pid, prefix, cls):
        handle = k.CreateToolhelp32Snapshot(flag, pid)
        if handle == invalid:
            raise c.WinError(c.get_last_error())
        try:
            entry = cls(); entry.dwSize = c.sizeof(cls)
            suffix = '' if prefix == 'Thread' else 'W'
            first = getattr(k, prefix + '32First' + suffix)
            next_ = getattr(k, prefix + '32Next' + suffix)
            ok = first(handle, c.byref(entry))
            while ok:
                yield cls.from_buffer_copy(entry)
                ok = next_(handle, c.byref(entry))
        finally:
            k.CloseHandle(handle)

    matches = [x.th32ProcessID for x in entries(2, 0, 'Process', Process)
               if x.szExeFile.lower() == '007firstlight.exe']
    if len(matches) != 1:
        raise SystemExit('Expected exactly one running 007FirstLight.exe; found ' + str(len(matches)))
    pid = matches[0]
    module = next(x for x in entries(0x18, pid, 'Module', Module)
                  if x.szModule.lower() == '007firstlight.exe')
    digest = hashlib.sha256(Path(module.szExePath).read_bytes()).hexdigest()
    if digest != EXPECTED_EXE:
        raise SystemExit('Unsupported EXE; no process-memory snapshot taken')
    base = module.modBaseAddr
    # PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION. No write/operation rights.
    handle = k.OpenProcess(0x10 | 0x1000, False, pid)
    if not handle:
        raise c.WinError(c.get_last_error())
    reads = 0

    def read(address, size):
        nonlocal reads
        if not address or not 0x10000 <= address < 0x0000800000000000 or not 0 < size <= 4096:
            return None
        buf = c.create_string_buffer(size); got = c.c_size_t()
        reads += 1
        if k.ReadProcessMemory(handle, address, buf, size, c.byref(got)) and got.value == size:
            return buf.raw
        return None

    def number(address, size=8):
        value = read(address, size)
        return int.from_bytes(value, 'little') if value is not None else None

    def typename(pointer):
        vt = number(pointer)
        if vt is None or not base <= vt < base + module.modBaseSize:
            return {'vtable': vt, 'type': None}
        col = number(vt - 8)
        raw = read(col, 24)
        if not raw:
            return {'vtable': vt, 'type': None}
        signature, _, _, td, _, self_rva = struct.unpack('<6I', raw)
        name = read(base + td + 16, 192) if signature == 1 and col == base + self_rva else None
        return {'vtableRva': hex(vt - base), 'type': name.split(b'\0')[0].decode('ascii', 'replace') if name else None}

    def reference(address, table, masks):
        raw = read(address, 16)
        if not raw:
            return None
        pointer, index, generation = struct.unpack('<QII', raw)
        # Bounded generation-table access, never scan the table.
        cell = number(table + index * 4, 4) if table and index < 0x1000000 else None
        result = {'pointer': pointer, 'index': index, 'generation': generation,
                  'tableCell': cell, 'matchingMasks': [mask for mask in masks if cell is not None and (cell & mask) == generation]}
        if pointer:
            result.update(typename(pointer))
        result['stableRead'] = read(address, 16) == raw
        return result

    try:
        masks = {}
        queried = 0
        for thread in entries(4, 0, 'Thread', Thread):
            if thread.th32OwnerProcessID != pid:
                continue
            queried += 1
            th = k.OpenThread(0x40, False, thread.th32ThreadID)
            if not th:
                continue
            try:
                info = BasicThread()
                if nt.NtQueryInformationThread(th, 0, c.byref(info), c.sizeof(info), None) >= 0:
                    tls = number(info.TebBaseAddress + 0x58) if info.TebBaseAddress else None
                    slot0 = number(tls) if tls else None
                    mask = number(slot0 + 0x1AC, 4) if slot0 else None
                    if mask is not None:
                        masks[mask] = masks.get(mask, 0) + 1
            finally:
                k.CloseHandle(th)
        samples = []
        for _ in range(3):
            root_before = number(base + ROOT_RVA)
            node = number(root_before + 0x10) if root_before else None
            ids = read(node + 0x30, 8) if node else None
            table = number(base + TABLE_RVA)
            begin = number(base + LIST_RVA); end = number(base + LIST_RVA + 8)
            valid_list = begin is not None and end is not None and end >= begin and (end - begin) % 16 == 0 and end - begin <= 16 * 64
            players = []
            if valid_list:
                for address in range(begin, end, 16):
                    ref = reference(address, table, masks)
                    if ref and ref['pointer']:
                        ptr = ref['pointer']
                        ref['nativeLookupId1AC'] = number(ptr + 0x1AC, 4)
                        ref['field1A8'] = number(ptr + 0x1A8, 4)
                        ref['actor18'] = reference(ptr + 0x18, table, masks)
                        ref['loadout188'] = reference(ptr + 0x188, table, masks)
                    players.append(ref)
            contexts = []
            local_characters = []
            for rva in [0x05D1B188, 0x05D1B190]:
                singleton = number(base + rva)
                local_characters.append({'globalRva': hex(rva), 'singleton': singleton,
                    'singletonType': typename(singleton) if singleton else None,
                    'reference': reference(singleton + 0x18, table, masks) if singleton else None,
                    'component': number(singleton+0x28) if singleton else None,
                    'componentType': typename(number(singleton+0x28)) if singleton else None,
                    'componentPlayerField200': number(number(singleton+0x28)+0x200,4) if singleton and number(singleton+0x28) else None})
            sequence = number(root_before+0x108) if root_before else None
            seq_begin = number(sequence+0x40) if sequence else None
            seq_end = number(sequence+0x48) if sequence else None
            seq_ids = [number(at,4) for at in range(seq_begin,seq_end,4)] if seq_begin is not None and seq_end is not None and 0 <= seq_end-seq_begin <= 4096 and (seq_end-seq_begin)%4 == 0 else None
            health = number(root_before+8) if root_before else None
            dead_begin = number(health+0x48) if health else None
            dead_end = number(health+0x50) if health else None
            dead_ids = [number(at,4) for at in range(dead_begin,dead_end,4)] if dead_begin is not None and dead_end is not None and 0 <= dead_end-dead_begin <= 16384 and (dead_end-dead_begin)%4 == 0 else None
            for rva, offset in [(0x6454120, 0xD0), (0x6454150, 0x140)]:
                ref = reference(base + rva, table, masks)
                extra = number(base + rva + 16)
                # Native getters dereference the interface at global+16 after
                # validating the sibling typed reference, not its object base.
                if ref and extra:
                    ref['nestedReference'] = reference(extra + offset, table, masks)
                    ref['nestedInterface'] = number(extra + offset + 16)
                contexts.append({'rva': hex(rva), 'reference': ref, 'interface': extra})
            samples.append({'root': root_before, 'node': node,
                            'rootType': typename(root_before) if root_before else None,
                            'nodeType': typename(node) if node else None,
                            'rootPointers': [{'offset': hex(i), 'pointer': number(root_before+i), 'targetType': typename(number(root_before+i))} for i in range(0,0x70,8)] if root_before else [],
                            'nodeBytes': read(node,0x90).hex() if node and read(node,0x90) else None,
                            'ids30_34': list(struct.unpack('<II', ids)) if ids else None,
                            'table': table, 'listBegin': begin, 'listEnd': end,
                            'listBoundsValid': valid_list, 'players': players,
                            'contexts': contexts, 'localCharacters': local_characters, 'sequencePlayerIds': seq_ids, 'nativeNotAliveIds': dead_ids, 'rootStable': number(base + ROOT_RVA) == root_before,
                            'nodeStable': number(root_before + 0x10) == node if root_before else False,
                            'idsStable': read(node + 0x30, 8) == ids if node else False,
                            'listBoundsStable': number(base + LIST_RVA) == begin and number(base + LIST_RVA + 8) == end})
            time.sleep(0.15)
        result = {'label': args.label, 'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                  'pid': pid, 'exeSha256': digest, 'base': base, 'readCalls': reads,
                  'threadsSeen': queried, 'tlsMaskCounts': masks, 'samples': samples,
                  'limits': 'Live reads, no suspension: not an atomic snapshot; masks sampled before structures. No native function calls or memory writes.'}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({'output': str(args.output), 'label': args.label,
                          'ids': [s['ids30_34'] for s in samples],
                          'nativePlayerCounts': [len(s['players']) for s in samples],
                          'readCalls': reads}))
    finally:
        k.CloseHandle(handle)


if __name__ == '__main__':
    main()
