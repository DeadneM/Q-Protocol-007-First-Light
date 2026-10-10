#!/usr/bin/env python3
"""Generate an x64 DXGI proxy from the build runner's *actual* System32 exports.

The generated .DEF has exactly the system DLL's exported names and ordinals;
the x64 assembly trampolines preserve Win64 integer and floating arguments.
The proxy resolves every real target lazily at first exported function call.
"""
import argparse
import json
import re
from pathlib import Path

# VS dumpbin /exports (MSVC x64). Ordinal, hint, RVA, name [+ annotation].
EXPORT_ROW = re.compile(
    r"^\s*(\d+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(\S+)(?:\s+.*)?$"
)

def parse_exports(data):
    exports = []
    for line in data.splitlines():
        match = EXPORT_ROW.match(line)
        if not match:
            continue
        ordinal, hint, rva, name = match.groups()
        if name.startswith("[") or name == "(NONAME)":
            name = None
        exports.append((int(ordinal), name))
    exports.sort()
    if len(exports) < 10 or len(exports) > 256:
        raise ValueError(f"Unexpected DXGI export count: {len(exports)}")
    ordinals = [o for o, _ in exports]
    if len(set(ordinals)) != len(ordinals):
        raise ValueError("Duplicate DXGI ordinal")
    names = [n for _, n in exports if n is not None]
    if len(set(names)) != len(names):
        raise ValueError("Duplicate DXGI name")
    for required in ("CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2"):
        if required not in names:
            raise ValueError(f"Mandatory DXGI entry missing: {required}")
    for n in names:
        if not re.fullmatch(r"[A-Za-z_?$@.][A-Za-z0-9_?$@.]*", n):
            raise ValueError(f"Cannot safely export unusual symbol: {n!r}")
    return exports

def write_files(exports, output):
    output.mkdir(parents=True, exist_ok=True)
    deffile = ["; Generated from Windows System32 dxgi.dll exports", "LIBRARY dxgi", "EXPORTS"]
    assembly = [
        "; Generated x64 ABI-preserving DXGI proxy thunks",
        "OPTION CASEMAP:NONE",
        "EXTERN DXGI_ResolveExport:PROC",
        ".code",
    ]
    rows = []
    for i, (ordinal, name) in enumerate(exports):
        fn = f"DXGI_Proxy_{i:03d}"
        if name is None:
            deffile.append(f"    {fn} @{ordinal} NONAME")
        else:
            deffile.append(f"    {name}={fn} @{ordinal}")
        assembly += [f"{fn} PROC", f"    mov r10d, {i}", "    jmp DXGI_LazyDispatch", f"{fn} ENDP"]
        rows.append(f'    {{{json.dumps(name) if name is not None else "nullptr"}, {ordinal}}},')
    # Win64 entry stack is 8 mod 16, sub C8h (200) => 0 mod 16.
    # Shadow space 00-1F; GP 20-38; XMM0-5 40-9F; pointer A0.
    # When restored, tail-jump carries original caller's stack args intact.
    assembly += [
        "DXGI_LazyDispatch PROC",
        "    sub rsp, 0C8h",
        "    mov qword ptr [rsp+20h], rcx",
        "    mov qword ptr [rsp+28h], rdx",
        "    mov qword ptr [rsp+30h], r8",
        "    mov qword ptr [rsp+38h], r9",
    ]
    for i in range(6):
        assembly.append(f"    movdqu xmmword ptr [rsp+{0x40+i*16:X}h], xmm{i}")
    assembly += [
        "    mov ecx, r10d",
        "    call DXGI_ResolveExport",
        "    mov qword ptr [rsp+0A0h], rax",
        "    mov rcx, qword ptr [rsp+20h]",
        "    mov rdx, qword ptr [rsp+28h]",
        "    mov r8, qword ptr [rsp+30h]",
        "    mov r9, qword ptr [rsp+38h]",
    ]
    for i in range(6):
        assembly.append(f"    movdqu xmm{i}, xmmword ptr [rsp+{0x40+i*16:X}h]")
    assembly += [
        "    mov r11, qword ptr [rsp+0A0h]",
        "    add rsp, 0C8h",
        "    jmp r11",
        "DXGI_LazyDispatch ENDP",
        "END",
    ]
    manifest = [
        "// Generated DXGI export manifest from the CI host System32.",
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "struct DXGI_ExportDescriptor { const char* name; std::uint16_t ordinal; };",
        f"inline constexpr DXGI_ExportDescriptor kDXGI_Exports[{len(exports)}] = {{",
        *rows,
        "};",
        f"inline constexpr std::size_t kDXGI_ExportCount = {len(exports)};",
    ]
    (output / "dxgi_proxy.def").write_text("\n".join(deffile) + "\n", encoding="utf-8")
    (output / "dxgi_proxy_exports.asm").write_text("\n".join(assembly) + "\n", encoding="utf-8")
    (output / "dxgi_proxy_manifest.h").write_text("\n".join(manifest) + "\n", encoding="utf-8")
    (output / "dxgi_proxy_exports.json").write_text(
        json.dumps([{"ordinal": o, "name": n} for o, n in exports], indent=2) + "\n",
        encoding="utf-8",
    )

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--dump", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)
    p.add_argument("--verify", type=Path, help="dumpbin /exports of built proxy")
    args = p.parse_args()
    exports = parse_exports(args.dump.read_text(encoding="utf-8", errors="replace"))
    if args.verify:
        actual = parse_exports(args.verify.read_text(encoding="utf-8", errors="replace"))
        if exports != actual:
            missing = sorted(set(exports) - set(actual))
            extra = sorted(set(actual) - set(exports))
            raise SystemExit(f"Proxy export/ordinal mismatch: missing={missing}, extra={extra}")
        print(f"DXGI proxy export ABI PASS: {len(exports)} exact named/ordinal entries.")
    else:
        write_files(exports, args.out)
        print(f"DXGI manifest created: {len(exports)} exports. Native x64 thunks generated.")

if __name__ == "__main__":
    main()
