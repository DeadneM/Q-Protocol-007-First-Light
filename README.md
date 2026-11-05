<p align="center">
  <img src="assets/ChatGPT%20Image%2018%20sept.%202026,%2018_57_16.png" alt="Q Protocol banner" width="100%">
</p>

<p align="center">
  <img src="docs/images/banner.webp" alt="Q Protocol - 007 First Light" width="100%">
</p>

# Q Protocol — 007 First Light

Q Protocol is an unofficial PC gameplay patch for **007 First Light** built around a small set of game-native actions and a configurable in-game overlay.

**Experimental A34:** the [playable-player core rebuild](docs/PLAYABLE_PLAYER_CORE.md)
replaces loadout-based readiness with a shared native-character/session gate for
AUTO and manual actions. A33 was rejected by the maintainer. A34 requires gameplay
acceptance in story and Tac Sim; v0.9.7 remains the accepted production base.

> [!IMPORTANT]
> **Read [`PROJECT_STATE.md`](PROJECT_STATE.md) first.** It is the authoritative current state.

## Current state

**Q Protocol v0.9.7** is the current stable public release, promoted from the
maintainer-accepted A28/A29 on 2 November 2026 through
[PR #8](https://github.com/DeadneM/Q-Protocol-007-First-Light/pull/8).

[Download v0.9.7](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/v0.9.7/Q-Protocol_v0.9.7.zip)
| [Release notes](packaging/RELEASE_NOTES_V0.9.7.md)
| [Successful Windows release build](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41480287380)

- November-compatible native core for Steam build 26252352, with PE/native-map
  compatibility checks retained. Older v0.9.6 targets the October executable.
- In-menu updater: startup check ON, beta builds OFF by default, manual Check now,
  confirmed Update on exit and cancellation. Existing explicit settings are kept.
- Only newer compatible releases are offered. Download hashes and binary layout
  are checked, loaded binaries are replaced after exit and backups are retained.
- Clearer responsive menu with accessible Save / Reload / Defaults.
- A26 authoritative DX12 queue capture, A27 bounded x64 graph scanner and A24
  request guards; existing native gameplay, F1-F12 and AUTO preserved.

**ZIP: only `dxgi.dll` and `QProtocol.asi` at root.** README, configuration source
and release history remain on GitHub. The ASI embeds default settings and the
updater helper. A missing INI is created; an existing personal INI is preserved.

Install with the game closed after backing up the two mod binaries and INI.
Extract beside `Retail/007FirstLight.exe`. A29 users can instead choose
**Updates > Check now > Update on exit**, then close normally after staging.
The actual post-exit installation cycle still awaits runtime confirmation;
transaction fixtures and published discovery/download checks passed separately.
The other configuration's GPU DEVICE_LOST incident remains open.

Previous v0.9.6, A28 and A29 releases are retained for history and compatible
rollback. See [A28 notebook](docs/A28_NOV2026_NATIVE_REMAP.md),
[A29 notebook](docs/A29_UPDATER_AND_MENU.md) and [history](docs/HISTORY.md).

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

## Experimental builds

[A38 compatibility test for Steam build 26430236](docs/A38_NOV5_REMAP.md)

[A37 test and gameplay checklist](docs/A37_WEAPON_CADENCE.md)

## Disclaimer

Q Protocol is an unofficial community modification and is not affiliated with or endorsed by the game publisher, developer, or rights holders.
