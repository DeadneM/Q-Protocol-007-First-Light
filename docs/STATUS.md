# Technical status

## v0.9.6 public release: A25 + native DXGI (2026-10-10)

- New public release v0.9.6 promoted from A25 per maintainer request, separate from historical v0.9.5.
- Floating ImGui menu defaults to 1000×850 constrained to current work area, with the Save / Reload / Defaults footer outside scrollable tab contents.
- Native x64 custom DXGI proxy is included in the four-file root ZIP and loads the matching QProtocol.asi alongside it. Back up existing local DXGI/ReShade/Special K wrappers before replacing.
- A24 gameplay, A23 adaptive 41-graph discovery, A22 mouse improvements, First Debug DX12 safety, weapon IDs and public QProtocol.ini are preserved.
- GitHub Actions audits C++ source isolation, Windows DXGI exported functions/ordinals, actual CreateDXGIFactory1 forwarding, overlay/API imports and ZIP root layout.
- v0.9.5 stays available for rollback; historical A23 crash root cause and wide GPU/Windows support are not proved by the build CI.
- Details: `packaging/RELEASE_NOTES_A25.md`. F1 close-combat research remains paused.

## v0.9.5 public release — A24

- User reports A24 appears to work in-game and requested publication.
- Runtime A23 log confirms graph index restored to 41 linked graphs, 42 ItemEntries, 43 spawners, with F3/AUTO and multiple weapon spawns completed.
- A21/A22 harden the overlay and mouse input; A23 repairs graph discovery around live player memory; A24 guards duplicate/burst requests (4 pending, 12 per rolling 30 seconds).
- F1/F2/F3/AUTO semantics, weapon IDs and `QProtocol.ini` defaults retained.
- Earlier A23 crash cause remains unproven, and cross-hardware compatibility still requires field feedback.
- `v0.9.4` is kept as the previous stable release. F1 close-combat research A20K–A20N remains paused.
- See `packaging/RELEASE_NOTES_A24.md`.

## A21 — archived overlay compatibility candidate (superseded by v0.9.5)

- F1 / close-combat research is paused.
- Active branch: `dev/a21-overlay-compat`.
- Gameplay/config base remains validated `v0.9.4 / A20J`.
- First Debug DX12 hardening is retained.
- Overlay is now a normal floating/movable/resizable ImGui window instead of a
  forced fixed-center panel.
- Input swallowing follows ImGui `WantCaptureMouse` /
  `WantCaptureKeyboard` / `WantTextInput` instead of consuming every input
  message while visible.
- Game cursor clipping is restored when the overlay closes/fails/shuts down.
- Run #131 / `37979873788`: **PASS**.
- Pre-release: `a21-test`.
- Status: **TEST / runtime validation pending**, not yet a public fix.
- Public `v0.9.4` remains untouched.
- Details: `A21_OVERLAY_COMPAT.md`.

## First Debug — public overlay compatibility investigation (2026-10-09)

- Report: users encounter a game crash on **Insert** (first display of the DX12 overlay); initial log is from A20I v0.9.3.
- Root cause unproven. Potential renderer first-frame initialization failure, swapchain mismatch, and unsafe DX12 resize/resource lifetime identified in source audit.
- Pinned ImGui and MinHook are built into the ASI; do not treat a missing end-user dependency as established.
- Fix on the development side, with safe fail-open overlay behavior. Preserve A20J gameplay functions, packaging and INI. **Do not require public users to run complicated tests.**
- Project notebook and developer acceptance criteria: **[First Debug](FIRST_DEBUG.md)**. Status: **OPEN**.

## Active state

- Current public release: **Q Protocol v0.9.6 / A25 Accessible Overlay + Native DXGI**.
- A20J is the validated post-update base for the October 7, 2026 executable.
- Player-ready resolution, F1, F2, F3, F4, F5-F12 and AUTO are validated in-game.
- Weapon spawning uses each RID's native ItemEntry/Spawner graph directly.
- F1 observes the game's native License To Kill state and forces the opposite state.
- Overlay tabs: Loadout / Weapons / Debug / Hotkeys.

- Behavioral reference: **U74** on the pre-update executable.
- Current post-update canonical build: **Fresh Core A20J**.
- Rejected compatibility branches: U80, U81, U81B, U82, U82A, U83, U84, U85.
- Gadgets: removed.
- Overlay/config model: locked.
- Source of truth: `/PROJECT_STATE.md`.
- **A20J Clean Core is validated in game and publicly released as v0.9.4**.
- A20J audit and validation are recorded in `docs/A20J_CLEAN_CORE_AUDIT.md`; no RID/gameplay/timing changes were introduced.

## Authoritative default controls

```text
F1  License To Kill
F2  Ammo +
F3  Manual weapon loadout
F4  Q-Pistol swap
F5  Weapon slot 1
F6  Weapon slot 2
F7  Weapon slot 3
F8  Weapon slot 4
F9  Weapon slot 5
F10 Weapon slot 6
F11 Weapon slot 7
F12 Weapon slot 8
```

Overlay toggle: **Insert by default, remappable from Hotkeys / `[Overlay] ToggleKey`**.

## Required core primitives

```text
ResolvePlayer()
ToggleLicenseToKill()
AddAmmo(player, profile)
ApplyWeaponLoadout(player, profile)
SwapQPistol(player)
GiveWeapon(player, weapon)
```

Manual and AUTO must call the same implementation.

## Manual / AUTO profiles

Both profiles are independently configurable.

Manual:
- `ManualLoadout.QPistol` chooses one Q-Pistol variant;
- `ManualLoadout.OneHanded` chooses one one-handed firearm;
- `ManualLoadout.TwoHanded` chooses one two-handed firearm;
- `ManualAmmo` chooses the ammo quantities F2 adds.

AUTO:
- `AutoLoadout` uses the same exact three typed roles;
- `AutoAmmo` chooses the ammo quantities AUTO adds.

The game loadout is never modeled as an arbitrary N-weapon list. F5-F12 are separate hotkeys, not loadout slots.

AUTO waits for the real player, then calls:

```text
ApplyWeaponLoadout(player, AutoLoadout)
AddAmmo(player, AutoAmmo)
```

When the player disappears or changes, AUTO resets.

Manual and AUTO use the same gameplay functions. No separate AUTO weapon/ammo engine is allowed.

## Overlay

Current runtime bindings are fixed by role:

```text
F1  LicenseToKill
F2  Manual Ammo
F3  ManualLoadout
F4  SwapQPistol
F5-F12 Weapon
```

F5-F12 weapon aliases are INI-backed. Generic F1-F12 `Action=` reassignment
is **not implemented yet** and the current `Action=` rows are descriptive.

The overlay toggle itself is remappable from the Hotkeys tab and persists to
`[Overlay] ToggleKey`.

Overlay controls include Save, Reload and Reset Defaults. INI remains the only
persisted configuration source.

## Primitive audit

Completed. See [`PRIMITIVE_AUDIT_OCT2026.md`](PRIMITIVE_AUDIT_OCT2026.md).

Key result: the October executable exposes a native `AddFirearmAmmunitionToPlayer` input, so F2/AUTO ammo no longer need the low-level reserve-vector setter.

The donor pair-clone path is retired. `GiveWeapon()` now resolves and triggers each weapon's native ItemEntry/Spawner graph directly.

## Next exact step

Fresh source implementation, in this order:

1. bootstrap/log/INI;
2. ResolvePlayer;
3. F1 LTK;
4. shared GiveWeapon queue;
5. F4 + F5-F12;
6. F3 ManualLoadout;
7. F2 ManualAmmo via native AddAmmo input;
8. AUTO via the same functions;
9. gameplay validation;
10. Insert overlay.


## Fresh Core A2 validation

Validated in-game on the October 2026 executable.

Confirmed working:
- clean ASI bootstrap;
- ResolvePlayer();
- F1 License To Kill;
- shared GiveWeapon();
- F4 Q-Pistol swap;
- F5-F12 configured weapon slots;
- minimal gameplay-thread native spawn hook.

A2 is now the canonical fresh-core base.

Next: corrected F3 ManualLoadout with exactly QPistol + OneHanded + TwoHanded, implemented only as a thin caller of the validated GiveWeapon queue.
