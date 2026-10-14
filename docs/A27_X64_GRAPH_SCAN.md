# Q Protocol A27 — x64 weapon graph scan range TEST

Date: 2026-10-14
Baseline: A26 `dev/a26-authoritative-swapchain-queue`, gameplay otherwise unchanged from v0.9.6.
Status: **TEST ONLY**, public v0.9.6 remains untouched.

## User report

The affected A26 user could open the overlay, but F3 ManualLoadout failed repeatedly:

```text
player loadout anchor=0x0000000080B60E50
A23 graph index: 0 linked graph(s), 0 item RID entry(s), 0 spawner(s)
[ERROR] GiveWeapon: source graph NOT FOUND for RID 019973508A5E327B
```

Review of `src/QProtocol.cpp` demonstrated a *definite scanner defect*: the
A23 player-relative memory scan was gated behind `anchor < 0x80000000`.
The observed live loadout pointer was `0x80B60E50`, just above that
arbitrary 2 GiB threshold. Consequently, the player-relative ±32 MiB
scan was skipped **entirely** and only the old fixed 64 MiB region
`0x2C000000..0x30000000` was inspected.

This defect explains why the user's graph scan was unable to inspect nearby
game objects. It does **not** prove those objects are present in that band
or that it causes the separate DX12 device loss.

## A27 change

- Keep the original fixed address scan and all object identification and
  native weapon routines unchanged.
- Derive maximum user-mode address using `GetNativeSystemInfo`.
- Check that the player loadout pointer is within the actual x64 user-mode
  address range, points into committed readable memory according to
  `VirtualQuery`, and only then scan ±32 MiB.
- Perform non-overflowing arithmetic and 64 KiB alignment; exclude regions
  already examined by the fixed-range scan.
- Reuse `VirtualQuery`/page-protection checks and bounded 64 KiB
  `ReadProcessMemory` chunks from A23.
- Log `A27 graph scan:` with pointer/scan bounds and fixed/local scan
  statistics (region counts, read calls, failures and bytes read).
- No changes in F1–F12, player resolver, loadouts, graph linking, RID
  conversion, stable config, or A26 rendering and queue capture.
- A26 still shows device loss `0x887A002B` on at least one affected PC.
  **GPU bug remains open**; A27 does not claim to fix that crash.

## Acceptance

1. CI source-isolation audit ensures the only runtime changes relative to
   A26 are within `BuildGraphIndex()`. A26 rendering source is byte-for-byte
   identical, as are the INI and native DXGI source.
2. Builds on Windows MSVC, direct x64 DXGI proxy export/ABI smoke tests,
   31 weapon catalogue and frozen config pass.
3. Four files are directly at test ZIP root: `dxgi.dll`,
   `QProtocol.asi`, `QProtocol.ini`, `README.txt`.
4. Field log should show for anchor `0x80B60E50` a nonempty local
   scan window near `0x7EB60000..0x82B70000` and local scan stats,
   plus item/spawner/linked graph counts.
5. Even if the scan returns objects, this is not a fix to GPU `DEVICE_LOST`.
   Do not promote A27 as a permanent fix until the affected user validates it.

## Executable identity caveat

The current mod accepts a game EXE if `TimeDateStamp=0x6AC4D653` and
`SizeOfImage=0x06D31000`. This is **not** full binary identity verification.
To establish whether two users have byte-identical EXEs, compare SHA-256 of
their `007FirstLight.exe`. No accusation of deceptive EXE claims is justified.
