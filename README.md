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

**A29 TEST development:** A28 was accepted on the maintainer system. A29 adds
startup update checking, beta releases and explicit installation after exit,
plus a clearer responsive menu. Native A28 gameplay/input/rendering stay frozen.
See [A29 notebook](docs/A29_UPDATER_AND_MENU.md) and
[release notes](packaging/RELEASE_NOTES_A29.md). [A29 TEST ZIP](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/a29-updater-menu-test/Q-Protocol_v0.9.6_A29_UpdaterMenu_TEST.zip) is built and independently checked. A29 gameplay and the real post-exit update cycle are not yet validated.

**New package policy (maintainer request, 2 November 2026):** from A29 onward,
the ZIP contains only `dxgi.dll` and `QProtocol.asi`. An existing personal INI is
kept; default settings are embedded and create an INI only when missing. README,
default INI and release history remain on GitHub. Older ZIPs stay unchanged.

**A28 TEST compatibility work:** the local Steam build 26252352 has been analyzed and individually remapped on `dev/nov2026-game-update-remap`. The new PE/native-map gate refuses mismatches before hooks. [A28 TEST ZIP](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/a28-nov2026-remap-test/Q-Protocol_v0.9.6_A28_Nov2026_Remap_TEST.zip) is built and package-checked; the maintainer reports A28 working locally. See [A28 notebook](docs/A28_NOV2026_NATIVE_REMAP.md). Public v0.9.6 is unchanged.

**Q Protocol v0.9.6 / A25 Accessible Overlay + Native DXGI** is the current public release.

> **November 2026 game-update notice:** A26 verified DXGI queue capture and A27 x64 weapon graph scanner are now integrated in `main` ([PR #7](https://github.com/DeadneM/Q-Protocol-007-First-Light/pull/7)). This is the source baseline, **not a public release validated for the newest game executable**. The public v0.9.6 ZIP targets the October 7, 2026 executable. The local updated executable has now been statically remapped for A28 TEST; gameplay acceptance is still pending. GPU `DEVICE_LOST` remains an open compatibility issue on some machines.

- Updated for the October 7, 2026 game executable.
- A25 enlarges the first-open floating overlay to 1000×850 ImGui units and keeps Save / Reload / Defaults accessible below a scrollable tab content area.
- Public ZIP now includes our custom native 64-bit `dxgi.dll` proxy to load `QProtocol.asi` directly.
- A24 received a positive in-game maintainer report and is promoted from TEST to public release.
- First Debug / A21 harden the DX12 renderer and provide a movable, resizable ImGui overlay.
- A22 improves click reliability and in-overlay mouse handling.
- A23 dynamically locates native weapon graphs around the live player loadout; 41 linked graphs were recovered in the A23 runtime log.
- A24 limits duplicate/repeated F4–F12 and Debug Spawn requests to 4 pending and 12 accepted in 30 seconds.
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

> **Compatibility note:** the root cause of the earlier A23 crash is not proven by its log. A24 mitigates a possible spawn-request overload; wider hardware validation is ongoing. Previous [v0.9.4](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/v0.9.4) remains available.

[Download latest release](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/latest)

## Native DXGI and package history

Starting with **A25 cumulative ZIPs**, future Q Protocol builds include our custom game-local `dxgi.dll` proxy **by default**. A25-A28 packages contained four root files. The user-approved A29 policy above supersedes that layout for future packages. The DXGI proxy loads the ASI and forwards Windows DXGI calls to System32. Back up any existing `dxgi.dll` (including ReShade/Special K) before installing.

This policy is recorded in [DXGI_PACKAGING_POLICY.md](docs/DXGI_PACKAGING_POLICY.md). **v0.9.6** is the first numbered public release to use this four-file layout. The earlier v0.9.5 archive is preserved for rollback.

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
- [`packaging/RELEASE_NOTES_A25.md`](packaging/RELEASE_NOTES_A25.md) — v0.9.6 cumulative release notes.
- [`packaging/RELEASE_NOTES_A24.md`](packaging/RELEASE_NOTES_A24.md) — v0.9.5 release notes.
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
