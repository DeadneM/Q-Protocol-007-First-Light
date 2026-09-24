<p align="center">
  <img src="assets/ChatGPT%20Image%2018%20sept.%202026,%2018_57_16.png" alt="Q Protocol banner" width="100%">
</p>

<p align="center">
  <img src="docs/images/banner.webp" alt="Q Protocol - 007 First Light" width="100%">
</p>

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

## Reproducible U49 build

The repository contains a dependency-free builder that reconstructs U49 from the canonical U33 `QProtocol.asi`.

Canonical U33 SHA-256:

```text
a0ac354d7c7f2c96ceae2fa3da6cadc0756d325d9fb796a4da2083a223f10dce
```

Expected U49 SHA-256:

```text
ba119fd8b781b15c1bc41bf393cddcd5dc7a3836ab9081426e4e6a01245a3a03
```

Build command:

```bash
python tools/build_u49.py /path/to/U33/QProtocol.asi -o QProtocol.asi
```

The builder validates the input hash, every binary preimage, and the final U49 hash.

## Default controls

| Key | Function |
|---|---|
| F1 | License To Kill toggle |
| F2 | Manual gadget / Q-Lens package |
| F3 | Manual reserve-ammo refill |
| F4 | Manual Q Protocol package |
| F5–F12 | Configurable native weapon actions |

## Repository layout

- `config/QProtocol.ini` — exact U49 configuration.
- `tools/build_u49.py` — reproducible U33 → U49 builder.
- `patches/U49_PATCH.md` — exact byte/RVA patch record.
- `docs/STATUS.md` — current technical state.
- `docs/HISTORY.md` — development lineage and rejected branches.
- `checksums/SHA256.txt` — current package/output hashes.
- `docs/images/banner.webp` — current project banner.
- `docs/images/archive/` — archived Q Protocol artwork.

The downloadable packaged build is generated from these verified inputs. The cumulative reverse-engineering notebook remains bundled in the ZIP build as `README.txt`.

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
