<p align="center">
  <img src="assets/ChatGPT%20Image%2018%20sept.%202026,%2018_57_16.png" alt="Q Protocol banner" width="100%">
</p>

<p align="center">
  <img src="docs/images/banner.webp" alt="Q Protocol - 007 First Light" width="100%">
</p>

# Q Protocol — 007 First Light

Q Protocol is an experimental PC gameplay patch for **007 First Light** focused on reusing game-native weapon, gadget, ability, ammo, and License To Kill systems.

> **Current integration/test build:** v0.8.12U67 — Player-Anchored Gadget List  
> **Canonical stable lifecycle fallback:** U57  
> **Known blocker:** the current gadget name/identity catalogue is not yet trustworthy enough for final promotion.

## Highlights

- Manual Q Protocol package on **F4**.
- Manual gadget / Q-Lens package on **F2**.
- Manual reserve-ammo refill on **F3**.
- License To Kill toggle on **F1**.
- Configurable F5–F12 native weapon actions.
- Automatic weapon package using the live runtime player.
- U49 proof: after the validated AUTO weapon package succeeds, the exact existing F2 gadget wrapper is called from the same gameplay frame.
- Game-native systems are reused wherever possible instead of reimplementing gameplay logic externally.

## Current post-update lineage

The 2026-09-25 game update invalidated the old U49 addresses. Q Protocol was fully rebased in U54, then the gadget lifecycle was stabilized in U57.

```text
U49 validated architecture
        ↓
U54 full game-update rebase
        ↓
U57 safe AUTO gadget lifecycle
        ↓
U59 identity-based four-target learning
        ↓
U60 corrected target-record order
        ↓
U67 player-anchored one-shot gadget census
```

### What is working in U67

- current game-update compatibility;
- manual F4 package and native weapon spawning;
- automatic weapon package;
- reserve-ammo infrastructure;
- abilities / Q-Lens path;
- License To Kill path;
- manual F2 reaches the normal native gadget path;
- AUTO uses the U57 safe loadout/player lifecycle;
- the U67 one-shot scanner resolves a complete four-target runtime-token set in the latest test.

### Remaining blocker

The actual gadget wheel is still wrong even though U67 reports a complete target mask.

Latest user log:

```text
U67 GadgetList loaded mask = 0x75
U67 GadgetList target mask = 0x0F
U67 GadgetList READY = 1
U67 Dartgun token = 0x0A010000
U67 SmokePellets token = 0x0A010001
U67 MissilePen token = 0x0A010008
U67 QuickHack token = 0x0A010002
```

Manual F2 also reaches the existing path:

```text
F2 native gadgets requested.
F2 native gadgets + Q-Lens applied directly.
```

Therefore the remaining problem is **identity data**, not whether F2 executes. Session-local `0x0A0100XX` tokens and resource RIDs must not be treated as interchangeable gadget identities.

## U57 canonical lifecycle

U57 fixed the post-update transition crash by removing the direct F2 call from weapon completion.

```text
AUTO native weapon set COMPLETE
        ↓
AUTO_GADGET_PENDING = 1
        ↓
validated live gadget loadout + runtime player READY
        ↓
synthesize existing F2 edge
        ↓
exact existing F2/native gadget path
```

U57 remains the stable lifecycle fallback while the identity catalogue is corrected.

## U60 identity mapping

U59 changed token learning from vanilla slot position to selected-object fingerprint identity for the four target gadgets.

U60 corrected the real native target-record order:

```text
0 = Left
1 = Up
2 = Down
3 = Right
```

Desired Q Protocol quartet:

```text
Left  = MissilePen
Up    = Dartgun
Down  = SmokePellets
Right = Quick Hack
```

## U67 architecture

U67 preserves U60's `.q31` remapper byte-for-byte and layers a one-shot read-only census around the validated runtime-player-ready object.

- scan anchor: actual `RUNTIME PLAYER READY object`;
- maximum window: ±512 MiB;
- `VirtualQuery` + `ReadProcessMemory`;
- committed readable pages only;
- dedicated 64-KiB scan buffer;
- one scan per player generation;
- no B-key diagnostic flow;
- manual F2 is not gated by scanner readiness;
- AUTO may wait only while the one-shot scan is running;
- scanner results populate the existing U60 identity-token table.

The exact binary comparison is documented in [`patches/U67_DELTA.md`](patches/U67_DELTA.md).

## Exact current hashes

U57 canonical lifecycle ASI:

```text
e0e3cf2df49d03b422fbdfcb868d3ca107619b26d004078d5b3f3eb65f31f192
```

U60 identity-mapping ASI:

```text
a35dacbca68b0b02b1b266b8d52e7e35718753fa4c511f1e328ff2d7a8300eaa
```

U67 current test ASI:

```text
8c6859628d64641fb0c57010f782b35307fa425364b22cc46e0aa70b9b7d07cb
```

U67 current test ZIP:

```text
66def588819f76926030a4ae46e60cd049d803003ec612d91b67c7d15309eec1
```

## Default controls

| Key | Function |
|---|---|
| F1 | License To Kill toggle |
| F2 | Manual gadget / Q-Lens package |
| F3 | Manual reserve-ammo refill |
| F4 | Manual Q Protocol package |
| F5–F12 | Configurable native weapon actions |

## Repository layout

- `config/QProtocol.ini` — packaged configuration.
- `patches/U67_DELTA.md` — exact U67 vs U57/U60 audit.
- `patches/U49_PATCH.md` — historical U49 byte-level patch record.
- `tools/build_u49.py` — historical reproducible U33 → U49 builder.
- `docs/STATUS.md` — current technical state.
- `docs/HISTORY.md` — cumulative development lineage and rejected branches.
- `checksums/SHA256.txt` — canonical/current hashes.
- `docs/images/banner.webp` — project banner.

## Development rules

Q Protocol deliberately favors surgical changes:

- preserve validated native paths;
- do not replace working hooks with broader rewrites without evidence;
- keep MANUAL and AUTO on shared primitives;
- distinguish in-game validation from log-only observations;
- keep rejected experiments documented;
- do not promote a test build to canonical until it survives actual gameplay testing.

## Important lineage

- **U30/U31:** validated native four-gadget remapper lineage.
- **U33:** historical pre-update canonical fallback.
- **U39:** identity/runtime census milestone.
- **U44–U48:** rejected AUTO integration experiments.
- **U49:** pre-update AUTO success → exact F2 architecture.
- **U54:** full rebase after the 2026-09-25 game update.
- **U57:** validated safe AUTO gadget lifecycle; current stable lifecycle fallback.
- **U59/U60:** identity-based four-target mapping and corrected record order.
- **U61:** diagnostic seven-gadget census / scanner infrastructure.
- **U62–U66:** rejected resolver/list experiments.
- **U67:** current working integration/test base; gadget identity catalogue still under audit.

## Disclaimer

Q Protocol is an unofficial community modification and is not affiliated with or endorsed by the game publisher, developer, or rights holders. Back up your files and test experimental builds at your own risk.
