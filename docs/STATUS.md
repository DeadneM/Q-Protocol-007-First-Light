# Technical status

## Current branch state

- **Canonical stable lifecycle fallback:** U57.
- **Current integration/test build:** U67.
- **U67 gameplay result:** core systems are working again; manual F2 executes; AUTO lifecycle executes; level/player readiness remains stable.
- **Known blocker:** gadget identity/name mapping is still wrong. U67 resolves a four-target runtime-token set mechanically, but the catalogue assigning those tokens to gadget names is not yet authoritative.
- **Promotion status:** U67 is accepted as the current working test/integration base, but is **not promoted over U57 as the final canonical gadget build** until all seven gadget identities are recaptured from explicit runtime selections.

## U57 validated lifecycle

U57 is the stable lifecycle foundation after the 2026-09-25 game update.

```text
AUTO native weapon set COMPLETE
        ↓
AUTO_GADGET_PENDING = 1
        ↓
validated live gadget loadout + runtime player READY
        ↓
synthesize the existing F2 edge
        ↓
exact existing F2/native gadget path
```

This fixed the U56 second-level crash caused by calling F2 before the new mission gadget loadout existed.

U57 ASI SHA-256:

`e0e3cf2df49d03b422fbdfcb868d3ca107619b26d004078d5b3f3eb65f31f192`

## U59 / U60 identity mapping

U59 changed token learning from vanilla slot position to selected-object fingerprint identity for four target gadgets.

U60 corrected the real target-record order:

```text
0 = Left
1 = Up
2 = Down
3 = Right
```

Desired mapping:

```text
Left  = MissilePen
Up    = Dartgun
Down  = SmokePellets
Right = Quick Hack
```

U60 ASI SHA-256:

`a35dacbca68b0b02b1b266b8d52e7e35718753fa4c511f1e328ff2d7a8300eaa`

## U61 diagnostic milestone

U61 proved that a non-equipped MissilePen can exist as a valid runtime object and token in TacSim. It also provided the read-only scanner infrastructure later reused by U67.

U61 itself remains diagnostic-only.

## Rejected U62–U66 experiments

U62–U65 accumulated post-F2 resolver/retry logic and were rejected.

U66 attempted a simpler prebuilt gadget list but anchored its bounded scan to the live mission loadout in a different memory region, producing an empty list and inadvertently gating manual F2. Rejected.

## U67 current integration base

U67 returns to U60 gameplay semantics and reuses only the proven U61 scanner infrastructure.

Changes:
- scanner anchored to the actual `RUNTIME PLAYER READY object`;
- one asynchronous read-only scan per player generation;
- ±512 MiB bounded scan;
- dedicated 64-KiB buffer;
- manual F2 left ungated;
- only AUTO may briefly wait for list construction;
- scanner result is published into the existing U60 identity-token table.

U67 ASI SHA-256:

`8c6859628d64641fb0c57010f782b35307fa425364b22cc46e0aa70b9b7d07cb`

U67 test ZIP SHA-256:

`66def588819f76926030a4ae46e60cd049d803003ec612d91b67c7d15309eec1`

Latest log:

```text
U67 GadgetList loaded mask = 0x75
U67 GadgetList target mask = 0x0F
U67 GadgetList READY = 1
U67 Dartgun token = 0x0A010000
U67 SmokePellets token = 0x0A010001
U67 MissilePen token = 0x0A010008
U67 QuickHack token = 0x0A010002
```

Manual F2 is confirmed to execute the native path, but the resulting gadget wheel is incorrect.

## Current diagnosis

The remaining problem is not AUTO readiness and not the F2 call path.

The scanner is publishing runtime tokens under gadget names that cannot yet be trusted. Stable resource RIDs and runtime `0x0A0100XX` tokens must not be treated as interchangeable identities.

## Next work

Recapture all seven gadget identities from explicit in-game selections/equips and record, for each gadget:

- selectedObject;
- `+0x108` fingerprint;
- `+0x110`;
- `+0x118`;
- `+0x120` runtime token;
- `+0x128` signature;
- resource/definition RID evidence where useful.

Only after that table is authoritative should the U67 publisher be corrected and promoted.
