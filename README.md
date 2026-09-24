# Q Protocol — 007 First Light

Q Protocol is an experimental PC gameplay patch for **007 First Light** focused on reusing game-native weapon, gadget, ability, ammo, and License To Kill systems.

> **Current test build:** v0.8.12U49 — Auto Weapon Success → Exact F2  
> **Canonical fallback:** U33. U49 has an initial positive in-game result, but it remains a test build until broader level/respawn validation is complete.

## Highlights

- Manual Q Protocol package on **F4**.
- Manual gadget / Q-Lens package on **F2**.
- Manual reserve-ammo refill on **F3**.
- License To Kill toggle on **F1**.
- Configurable F5–F12 native weapon actions.
- Automatic weapon package using the live runtime player.
- U49 proof: after the validated AUTO weapon package succeeds, the exact existing F2 gadget wrapper is called from the same gameplay frame.
- Game-native systems are reused wherever possible instead of reimplementing gameplay logic externally.

## U49 architecture

U49 is deliberately small. It is rebuilt from the U33 canonical ASI and does **not** reuse the rejected U44–U48 gadget-auto experiments.

```text
AUTO weapon package succeeds
        ↓
exact MANUAL F2 wrapper
        ↓
Q-Lens + GadgetActivation
        ↓
native four-record gadget spawner
        ↓
U30/U31 runtime-token remapper
```

The legacy `AutoSpawnGadget` direct writer remains disabled in U49 so it cannot compete with this path.

## Current status

### Working / previously validated

- Manual F4 package and weapon spawning.
- Manual F2 gadget path in known-good levels.
- Four usable gadget slots through the U30/U31 remapper.
- Q-Lens / gadget activation path.
- Runtime pair-clone weapon spawning.
- Manual and automatic ammo infrastructure.
- License To Kill research / work-in-progress path.

### U49 preliminary result

The first user test reported that U49 **appears to work**: AUTO weapons run, then the existing F2 gadget path is invoked automatically. More testing is required before promoting U49 over U33 as the canonical stable base.

### Known limitation

The U30/U31 gadget remapper still learns runtime gadget tokens by **vanilla slot position**, not by gadget identity. This can produce the wrong quartet in levels whose vanilla gadget layout differs. U39 proved an identity-based runtime-token approach is possible, including resolving a non-equipped MissilePen token, but that work is deliberately not merged into U49 yet.

## Installation

1. Back up your existing Q Protocol files.
2. Extract the current ZIP from `dist/`.
3. Copy `QProtocol.asi` and `QProtocol.ini` to the game's ASI/mod location.
4. Do not mix old experimental diagnostic ASIs with the current build.
5. Review `QProtocol.ini` before launching.

## Default controls

| Key | Function |
|---|---|
| F1 | License To Kill toggle |
| F2 | Manual gadget / Q-Lens package |
| F3 | Manual reserve-ammo refill |
| F4 | Manual Q Protocol package |
| F5–F12 | Configurable native weapon actions |

## Repository layout

- `bin/QProtocol.asi` — current U49 test binary.
- `config/QProtocol.ini` — current U49 configuration.
- `docs/STATUS.md` — concise technical state and lineage.
- `checksums/SHA256.txt` — U49 package hashes.
- `dist/` — packaged test build. The ZIP also contains the full cumulative reverse-engineering notebook as `README.txt`.

## Development rules

Q Protocol deliberately favors surgical changes:

- preserve validated native paths;
- do not replace working hooks with broader rewrites without evidence;
- keep MANUAL and AUTO on shared primitives;
- distinguish in-game validation from log-only observations;
- keep rejected experiments documented;
- do not promote a test build to canonical until it survives actual gameplay testing.

## Important lineage

- **U30:** four-gadget native remapper validated in game.
- **U31:** integrated the U30 remapper into Q Protocol.
- **U33:** canonical gadget fallback, manual-only with legacy AUTO gadget disabled.
- **U39:** identity-based runtime-token census proved the cross-level problem can be solved by identity.
- **U44–U48:** rejected AUTO gadget integration experiments.
- **U49:** rebuilt from U33; AUTO weapon success calls the exact MANUAL F2 wrapper.

## Disclaimer

Q Protocol is an unofficial community modification and is not affiliated with or endorsed by the game publisher, developer, or rights holders. Back up your files and test experimental builds at your own risk.
