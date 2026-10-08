<p align="center">
  <img src="assets/ChatGPT%20Image%2018%20sept.%202026,%2018_57_16.png" alt="Q Protocol banner" width="100%">
</p>

<p align="center">
  <img src="docs/images/banner.webp" alt="Q Protocol - 007 First Light" width="100%">
</p>

# Q Protocol — 007 First Light

Q Protocol is an experimental PC gameplay patch for **007 First Light** built around a small set of game-native actions and a configurable in-game overlay.

> [!IMPORTANT]
> **Read [`PROJECT_STATE.md`](PROJECT_STATE.md) first.** It is the authoritative current state.

## Current state

**Q Protocol v0.9.4 / Fresh Core A20J Clean Core** is the current public release.

- Updated for the October 7, 2026 game executable.
- Player/loadout resolution is remapped for the new executable.
- Weapon spawning now uses each weapon's native ItemEntry/Spawner graph directly.
- The obsolete donor pair-clone path is no longer used.
- Removed obsolete donor/pair-clone restoration code and dead overlay state.
- Q Protocol's own `WriteProcessMemory` import is removed in A20J; `ReadProcessMemory` and `VirtualProtect` remain where required.
- The Weapons tab provides configurable Weapon 1-8 slots for F5-F12.
- The previous catalogue/spawn tools are kept under Debug.
- F1 reads the game's native License To Kill state and forces the opposite state.
- The A15 weapon catalogue audit is still enforced automatically by CI.
- Experimental weapons remain in the catalogue but are hidden from Manual/Auto lists by default.
- The overlay can expose Experimental entries when testing is desired.
- A20J is the validated post-update Clean Core base; U74 remains a historical behavioral/reference library.
- Gadget gameplay code remains removed from the active Q Protocol direction.

## Default controls

| Key | Default action |
|---|---|
| F1 | License To Kill ON/OFF |
| F2 | Ammo + |
| F3 | Manual weapon loadout |
| F4 | Q-Pistol swap |
| F5-F12 | Configurable weapon slots |

## Overlay

Default overlay key: **Insert**, remappable from the **Hotkeys** tab and stored in
`[Overlay] ToggleKey`.

Current hotkey behavior is deliberately simple:

- F1 = License To Kill
- F2 = Manual ammo
- F3 = Manual loadout
- F4 = Q-Pistol swap
- F5-F12 = weapon slots whose weapon aliases are read from the INI

The `Action=` entries in the INI currently document those fixed semantics.
**Generic F1-F12 Key -> Action reassignment is not implemented yet.**

The overlay currently provides:

- Manual and Automatic loadout/ammo editors
- Weapons tab with configurable Weapon 1-8 slots for F5-F12
- F1 now toggles the actual native License To Kill state at press time
- Debug tab with weapon catalogue status editing, Spawn Weapon testing and runtime discovery
- Experimental weapon visibility toggle
- overlay-key capture/remapping
- Save / Reload / Reset Defaults
- player-ready state

Settings persist in [`config/QProtocol.ini`](config/QProtocol.ini).

See [`docs/OVERLAY_PLAN.md`](docs/OVERLAY_PLAN.md).

## Minimal core

```text
ResolvePlayer()
ToggleLicenseToKill()
AddAmmo(player, profile)
ApplyWeaponLoadout(player, profile)
SwapQPistol(player)
GiveWeapon(player, weapon)
```

Manual and AUTO share these exact primitives.

### Manual and AUTO profiles

Both profiles are editable independently in the overlay and INI.

**Manual**
- choose one Q-Pistol variant;
- choose one one-handed firearm;
- choose one two-handed firearm;
- choose the ammo quantities used by F2.

**AUTO**
- choose the same three typed weapon roles independently;
- choose the automatic ammo quantities.

F5-F12 remain standalone weapon hotkeys. They are not extra loadout slots.

```text
player available
    -> ApplyWeaponLoadout(player, AutoLoadout)
    -> AddAmmo(player, AutoAmmo)
    -> AutoDone = true
```

If the player disappears or changes, `AutoDone` resets.

AUTO is a trigger/profile selection layer, not a second gameplay implementation.

## Repository

- [`PROJECT_STATE.md`](PROJECT_STATE.md) — authoritative project state.
- [`docs/STATUS.md`](docs/STATUS.md) — concise technical status.
- [`docs/OVERLAY_PLAN.md`](docs/OVERLAY_PLAN.md) — overlay and input architecture.
- [`config/QProtocol.ini`](config/QProtocol.ini) — target configuration.
- [`docs/HISTORY.md`](docs/HISTORY.md) — historical notebook.
- [`docs/archive/`](docs/archive/) — retired gadget-era research.

## Development rules

- Keep the core small.
- Manual and AUTO share gameplay primitives.
- No gadget code.
- No duplicate manual/AUTO state machines.
- Map native functions by full semantic evidence, not guessed RVA deltas.
- Do not build another numbered compatibility release until the required primitives are understood.

## Disclaimer

Q Protocol is an unofficial community modification and is not affiliated with or endorsed by the game publisher, developer, or rights holders.
