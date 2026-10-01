<p align="center">
  <img src="assets/ChatGPT%20Image%2018%20sept.%202026,%2018_57_16.png" alt="Q Protocol banner" width="100%">
</p>

<p align="center">
  <img src="docs/images/banner.webp" alt="Q Protocol - 007 First Light" width="100%">
</p>

# Q Protocol — 007 First Light

Q Protocol is an experimental PC gameplay patch for **007 First Light** built around a deliberately small set of game-native actions.

> [!IMPORTANT]
> **Read [`PROJECT_STATE.md`](PROJECT_STATE.md) first.**  
> It is the authoritative source for the active architecture, rejected branches, and next exact step.

## Current state

The October 2026 game update changed the executable and invalidated the previous cumulative compatibility work.

- **Behavioral reference:** U74 on the pre-update executable.
- **Current post-update build:** none validated yet.
- **Rejected compatibility experiments:** U80 through U85.
- **Gadgets:** removed from Q Protocol. The game now handles them natively.
- **F2:** free.
- **Next task:** audit and map the four minimal primitives before producing another numbered build.

## Minimal architecture

Q Protocol now has only four required gameplay primitives:

```text
ResolvePlayer()
ToggleLicenseToKill()
GiveWeapon(player, weapon)
AddAmmo(player, profile)
```

Manual and AUTO must call the **same implementation**.

### Controls

| Key | Action |
|---|---|
| F1 | Toggle License To Kill |
| F2 | Free / unassigned |
| F3 | Add/refill configured ammo |
| F4–F12 | Give the configured weapon if the player is available |

### AUTO

AUTO is intentionally simple:

```text
player = ResolvePlayer()

if no player:
    AutoDone = false
    return

if player changed:
    AutoDone = false

if not AutoDone:
    call the same GiveWeapon/AddAmmo primitives using [Auto] values
    AutoDone = true
```

No separate AUTO implementation is allowed unless the current game executable proves it is strictly necessary.

## What is not coming back

The active implementation must not reintroduce:

- gadget remappers;
- gadget scanners;
- gadget producer hooks;
- F2 gadget logic;
- duplicate manual/AUTO weapon systems;
- duplicate manual/AUTO ammo systems;
- multiple player-readiness lanes;
- special Q-Pistol state machines if generic `GiveWeapon()` can handle it;
- old cumulative state machines merely because they existed in U74.

## Configuration target

The active configuration is kept in [`config/QProtocol.ini`](config/QProtocol.ini).

It describes the **target minimal core**, not the old cumulative U74 parser.

## Documentation

- [`PROJECT_STATE.md`](PROJECT_STATE.md) — current source of truth.
- [`docs/STATUS.md`](docs/STATUS.md) — concise technical status.
- [`docs/HISTORY.md`](docs/HISTORY.md) — historical development notebook.
- [`docs/archive/`](docs/archive/) — retired gadget-era and old patch documentation.
- [`checksums/SHA256.txt`](checksums/SHA256.txt) — current reference hashes.

## Development rules

- Keep the core small.
- Manual and AUTO share the same primitives.
- Map native functions by semantics/full-function evidence, not by guessed RVA deltas.
- Do not promote a build until it survives real gameplay testing.
- Update `PROJECT_STATE.md` after every meaningful accepted/rejected build.
- Preserve detailed old research in the archive, but never let it override the active state.

## Historical note

The project previously explored extensive gadget remapping and multi-stage AUTO logic through U30–U77. That research is preserved under `docs/archive/` and `docs/HISTORY.md`, but it is no longer part of the product direction.

## Disclaimer

Q Protocol is an unofficial community modification and is not affiliated with or endorsed by the game publisher, developer, or rights holders. Back up your files and test experimental builds at your own risk.
