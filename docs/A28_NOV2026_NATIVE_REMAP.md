# A28 November native remap TEST — 2026-11-01

## Target and evidence boundary

Local Steam build **26252352**, installed file dated October 30, 2026:
65,052,552 bytes, x64 PE, timestamp `0x6AE0DF3C`, image size `0x06D31000`,
SHA-256 `6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1`.
"November" is the development branch name, not a verified publisher patch date.
Old October EXE unavailable (maintainer confirmed); no full binary diff is claimed.
Existing installed logs date October 14 and cannot validate the new executable.

Base: PR #7, integrated A26/A27 `06245ba6623f590d165c05d227c078f2072831e5`;
branch originally checked out at `65098968228bc829ca5b83a9869a16a02d3512d5`.
Public v0.9.6/tag remains separate and immutable.

## Static remap

| Native symbol | October source RVA | A28 TEST RVA |
|---|---:|---:|
| Player resolver | 0171C2B0 | 0171D2E0 |
| Registry lookup | 007D6E20 | 007D6E80 |
| Loadout type registry | 067925D8 | 06792438 |
| Runtime player global | 06452838 | 06452BF8 |
| Ammo owner global | 06452820 | 06452BE0 |
| Player loadout primary vtable | 02EDD108 | 02EDD278 |
| LTK natural false / true | 0191A6B4 / 0191A71B | 0191B694 / 0191B6FB |
| LTK observer / true / false branch | 0191A6D8 / 0191A6E7 / 0191A763 | 0191B6B8 / 0191B6C7 / 0191B743 |
| ItemEntry primary vtable | 02EC97C8 | 02ECA0A8 |
| Spawner primary vtable | 02EC9990 | 02ECA270 |
| Native spawn | 016B4E80 | 016B5E90 |
| Native ammo insert / publish | 00115820 / 012A7330 | 001157B0 / 012A8470 |
| Gameplay epilogue | 0194BC01 | 0194CBD1 |

These are **static identifications for an experimental test**, not maintainer
gameplay validation. No global RVA delta was applied.

- LTK observer's complete 15-byte sequence is unique in executable sections.
  Decoded `test dl,dl; je` targets `0191B743`; fallthrough is `0191B6C7`.
  Natural `xor dl,dl` and `mov dl,1` paths are at `0191B694` and `0191B6FB`.
- MSVC RTTI CompleteObjectLocators identify primary vtables for the exact three
  classes. Player loadout also has a secondary vtable at `02EDD240` (offset 24),
  which must not replace its primary vtable at `02EDD278` (offset zero).
- Reflection name `ZKntPlayerLoadoutEntity` at `02ED6D01`, descriptor pointer
  at `03740FB0`, links its type registry at `03740FB8` to `06792438`.
  Native loadout lookup code at `0171CB0F` passes that same registry to
  `007D6E80`. The resolver is referenced by a vtable slot at `02EDD5D8`.
- Native ammunition caller at `015D5A97` loads owner `06452BE0`, builds the
  12-byte record, locks `+238E0`, inserts at `+20B70` via `001157B0`, publishes
  event `1DE` with context `+20AD0` via `012A8470`, then unlocks. Offsets and
  event remain unchanged. Runtime-player global `06452BF8` is independently
  referenced by native gameplay code; existing `+10 -> +30` READY/identity path
  remains subject to the live test.
- Spawn entry `016B5E90` takes the spawner in RCX, checks its entity handle,
  pending vector `+38/+40` and state byte `+50`, walks graph vector `+18/+20`,
  submits native work and sets state `+50`. ItemEntry constructor retains RID
  field `+120`. Reflection sizes remain ItemEntry `128` and Spawner `60` hex.
- The short gameplay preimage appears **41 times**. The selected epilogue
  `0194CBD1` is the final epilogue in native gameplay routine
  `0194BCF0..0194CBE4`, and includes the complete original stack/register restore
  through RET. Nearby candidates were not accepted just for matching bytes.
  The callback's correct execution context still requires live confirmation;
  without the old EXE, full old/new function equivalence cannot be claimed.

## Compatibility and preservation

PE refusal retained for other timestamps/image sizes. A28 additionally verifies
12 code windows, including native helpers, LTK paths, full gameplay epilogue and
the ammo publication callsite; five relocated type/function links are checked.
All must pass before native function setup or hooks. Existing hook preimage
checks remain. A modified target is refused; checks are never forced off.

`tools/verify_a28_scope.py` normalizes only the documented map, added gate and
log identity, then compares all remaining core source against integrated
A26/A27. Overlay A25/A26, scanner A27, assembly, native DXGI, INI, gameplay,
hotkeys, AUTO and spawn guards are preserved. `GameplayHook.asm` has historical
October comments; its replayed instructions remain unchanged and checked.

Raw static evidence and manifest are in `evidence/nov2026/`. The initial
`executable-audit.json` was generated against the **October source map** before
editing; its source hash records that boundary. The audit tool only reads files.
Disassembly used pefile 2024.8.26 and Capstone 5.0.9 locally; no EXE is committed.

## Acceptance and rollback

Objective: restore A26/A27 behavior on the identified EXE without widening runtime
changes beyond native mapping and compatibility checks. Success requires the
maintainer to confirm READY, F1, all six F2 ammo classes, three-role F3, F4/F5-F12,
AUTO/player reload and overlay input/Save/Reload, with both current-session logs.
Do not treat CI, static matches, a PASS log or graph counts alone as success.
GPU DEVICE_LOST `0x887A002B` remains OPEN on the other user's configuration.

Build is TEST only. Four files at ZIP root. Back up installed mod files, retain
personal INI, require launch approval, stop on an unexpected result. Restore the
backup on failure; old mod packages also require a matching old executable.

## TODO

- DONE: Windows CI 41357519256 SUCCESS; native DXGI export parity/smoke, source preservation, AMD64 ASI, dependency audit and flat ZIP. Downloaded ZIP 527319 bytes, SHA-256 `93eed2c59d71e7349512aa717fd16b39802bcf6db87f2431c286385be604334e`. INI identical; full hashes in `evidence/nov2026/a28-package-verification.json`.
- Obtain explicit launch approval for one maintainer runtime test.
- Review both fresh logs and actual gameplay outcomes; never infer acceptance.
- Resolve any READY/callback/ABI issue before a broader test or stable promotion.
- Keep the affected-user GPU incident separate; no universal crash-fix claim.
