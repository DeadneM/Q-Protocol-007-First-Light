# A23 — Dynamic Weapon Graph Recovery

Date: 2026-10-09
Status: **CI #137 PASS / runtime validation pending**
Branch: `dev/a23-dynamic-weapon-graph`
Baseline: A22 input compatibility + A21 floating UI + First Debug DX12 protections + A20J validated gameplay.

## Broken A22 runtime evidence

User supplied `QProtocol(3).log`:
- game executable accepted: TimeDateStamp `0x6AC4D653`, SizeOfImage `0x06D31000`;
- DX12 ImGui ready, first frame submitted, three left-button-down events observed;
- player READY several times; loadout addresses `0x2BDA8038`, `0x2BD70E50`;
- graph index always **0 linked ItemEntry/Spawner graphs, 1 spawner candidate**;
- all AUTO, F3, F4, F5-F12 and Debug Spawn requests eventually fail with `source graph NOT FOUND`;
- F1 native state toggling and F2 native ammo publication are logged; their actual gameplay effect was not proven by this file.

A previously successful `QProtocol(2).log` found **41 linked graphs, 43 spawner candidates**, with known Q-Pistol graph
`item=0x2CE59DC8`, `spawner=0x2CE59D30`, and player loadout around `0x2D210E50`.

The gameplay source scans only fixed addresses `[0x2C000000, 0x30000000)`.
Because the failing player's loadout is below that lower bound, a shifted heap layout is a plausible explanation for zero graphs.
This is **strong evidence for a discovery-range fault, not direct proof** that current native ItemEntry objects reside at a specific new address.

## A23 source correction

The core now scans:
1. The original known-good fixed window (unchanged).
2. The region within **32 MiB above/below the resolved live player loadout** that lies outside the original window.
3. Neither overlapping region is scanned twice.
4. RID reading, spawner graph associations, `ValidateGraph`, native `GiveWeapon`, queue timings, retry policy, and AUTO behavior remain unchanged.

The failure log gains:
```
A23 graph index: <linked> linked graph(s), <items> item RID entry(s),
<spawners> spawner(s), player loadout anchor=...
```

This distinguishes absence of native ItemEntry objects from failure to link ItemEntries to spawner lists.

## CI safeguards

A23 must preserve exactly, against tag `v0.9.4`:
- `src/GameplayHook.asm`
- `src/Overlay.h`
- `config/QProtocol.ini`

An additional source-isolation check permits the player-loadout scan anchor and the body of `BuildGraphIndex()` in `QProtocol.cpp`, **nothing else**. A synthetic relocation test models the offset between the prior working ItemEntry and loadout and verifies that the moved ItemEntry would fall under A23's adaptive scan rather than the old fixed range.

A22 input improvements are retained. A21 floating UI, First Debug shaders, native WndProc/hook protections, and package layout are retained.
F1 close-combat research (A20K-A20N) remains paused.

## Validation

A CI pass is *not* a claim of runtime success.

First checks:
- `A23 graph index` should show nonzero linked graph count and include requested Q-Pistol RID;
- AUTO and F3 must complete the weapon roles; F4, F5-F12 and Debug Spawn must operate;
- F1 and F2 must retain prior working behavior; overlay clicks and Insert remain reliable;
- no excessive memory scanning, freezes or spurious spawner actions.

If linked graphs remain zero, the next audit must inspect the new `item RID entry(s)` and `spawner(s)` counts; do **not** randomly change IDs/RIDs.

Stable public `v0.9.4` and previous A21/A22 test releases are preserved.
The ZIP remains flat with `QProtocol.asi`, `QProtocol.ini`, `README.txt`.

## Final CI/package

- GitHub Actions run #137 / `37989415687` SUCCESS
- QProtocol.asi 869888 bytes SHA-256 `a58454d6f6a365c47e4c191152536c97a37d3e6735583a6d6d5ba4a3706a511f`
- QProtocol.ini 6717 bytes SHA-256 `8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022`
- A23 test ZIP 451137 bytes SHA-256 `daf91b4fca1bbdc07de15fbd67f7f06fa0d87757c4a75303e8a794d9b0f79e3d`
- Release tag `a23-test`. No in-game validation yet.
