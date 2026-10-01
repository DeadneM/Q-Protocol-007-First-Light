# Q Protocol — Project State

> **Primary source of truth.**
> Read this file first when resuming Q Protocol in a new conversation.
> If older README/history notes conflict with this file, this file wins.

## Fresh Core A1 test candidate

- Build: `Q-Protocol_FreshCore_A1_ResolvePlayer_LTK_TEST.zip`
- Status: **partially validated**
- Scope:
  - fresh ASI bootstrap;
  - clean launch log;
  - October executable validation;
  - shared `ResolvePlayer()`;
  - F1 License To Kill toggle.
- F2-F12, AUTO, weapons, ammo and overlay are intentionally inactive in A1.
- ASI SHA-256: `153e65273f6a9c7b6b1dc351897aa8a1f9c8138dc96428c06bb1a299ee56cf68`
- ZIP SHA-256: `613fec2ce3c3dda6abac604431fede32fa626cf98c425fff48ff2e3bd62c96fc`
- LTK validation: PASS (user confirmed on 2026-10-02).
- ResolvePlayer runtime validation: still to be confirmed independently from log/player-dependent actions.
- Next: add one shared GiveWeapon() implementation, then F4 swap + F5-F12 slots.

## Current status

- Historical behavioral reference: **U74** on the pre-October-2026 executable.
- Current game executable: October 2026 update.
- Current post-update canonical build: **none yet**.
- U80, U81, U81B, U82, U82A, U83, U84 and U85 are rejected compatibility experiments.
- Never use U80-U85 as a new base.
- Gadget support is removed from Q Protocol and must not be reintroduced.
- Active repository surface has been cleaned; gadget-era material is archived under `docs/archive/`.

## Default controls

These defaults are now authoritative:

| Key | Default action |
|---|---|
| F1 | License To Kill ON/OFF |
| F2 | Add/refill ammo |
| F3 | Apply manual weapon loadout |
| F4 | Swap Q-Pistol mode |
| F5 | Weapon slot 1 |
| F6 | Weapon slot 2 |
| F7 | Weapon slot 3 |
| F8 | Weapon slot 4 |
| F9 | Weapon slot 5 |
| F10 | Weapon slot 6 |
| F11 | Weapon slot 7 |
| F12 | Weapon slot 8 |

## Overlay plan

Q Protocol will use an in-game overlay inspired by the clean Saboteur/Postal plan.

### Overlay key

- Default overlay toggle: **Insert**
- Overlay key is independent from F1-F12 so no gameplay slot is consumed.

### Slot model

Every F1-F12 key is represented as a generic configurable slot:

```text
Key -> Action -> contextual parameters
```

The default mapping is the table above, but the overlay may reassign any slot later without recompiling the ASI.

Supported action categories for the new core:

```text
None
LicenseToKill
Ammo
ManualLoadout
SwapQPistol
Weapon
```

Only parameters relevant to the selected action should be shown.

Examples:

- `LicenseToKill`: no extra parameter.
- `Ammo`: choose Manual/Auto ammo profile or explicit profile.
- `ManualLoadout`: choose configured loadout profile.
- `SwapQPistol`: choose the two Q-Pistol variants/modes.
- `Weapon`: choose one weapon alias/RID.

### Overlay controls

The overlay must provide:

- **Manual profile editor** -> choose ManualLoadout weapons and ManualAmmo quantities
- **AUTO profile editor** -> choose AutoLoadout weapons and AutoAmmo quantities
- **Save** -> write current settings to `QProtocol.ini`
- **Reload** -> reload `QProtocol.ini` from disk
- **Reset Defaults** -> restore the authoritative F1-F12 defaults and profile defaults
- current player-ready state
- current AUTO state
- current action assigned to each key

No second configuration system should exist outside the INI.

## Minimal gameplay core

The new ASI should expose only a small set of shared primitives:

```text
ResolvePlayer()
ToggleLicenseToKill()
AddAmmo(player, profile)
ApplyWeaponLoadout(player, profile)
SwapQPistol(player)
GiveWeapon(player, weapon)
```

### Rules

- F1 calls `ToggleLicenseToKill()`.
- F2 calls `AddAmmo(player, ManualAmmo)`.
- F3 calls `ApplyWeaponLoadout(player, ManualLoadout)`.
- F4 calls `SwapQPistol(player)`.
- F5-F12 call `GiveWeapon(player, configuredWeapon)`.
- All player-dependent actions first use the same `ResolvePlayer()`.
- No separate manual/AUTO implementation of the same gameplay primitive.
- A tiny weapon queue/busy flag is allowed only if the native spawner is asynchronous.

## Manual and AUTO profiles

Manual and AUTO are two configurable data profiles using the same gameplay code.

### Manual profile

The user can choose:

- the weapons contained in `ManualLoadout`;
- the reserve-ammo quantity for each supported ammo class in `ManualAmmo`.

Default behavior:

```text
F2 -> AddAmmo(player, ManualAmmo)
F3 -> ApplyWeaponLoadout(player, ManualLoadout)
```

### AUTO profile

The user can choose independently:

- the weapons contained in `AutoLoadout`;
- the reserve-ammo quantity for each supported ammo class in `AutoAmmo`.

AUTO remains conceptually simple:

```text
player = ResolvePlayer()

if no player:
    AutoDone = false
    return

if player changed:
    AutoDone = false

if not AutoDone:
    ApplyWeaponLoadout(player, AutoLoadout)
    AddAmmo(player, AutoAmmo)
    AutoDone = true
```

Manual and AUTO call the **same** `ApplyWeaponLoadout()`, `GiveWeapon()` and `AddAmmo()` implementations. Only weapon selections and ammo quantities differ.

AUTO does not own a separate state machine for weapons or ammo.

## Q-Pistol rule

F4 is specifically the Q-Pistol swap/toggle action.

The new implementation should prefer one simple `SwapQPistol()` path that switches between the configured Q-Pistol variants.

Do not resurrect the old special Q-Pistol package state machine.

## Features to keep

- F1 License To Kill toggle
- F2 ammo
- F3 manual weapon loadout
- F4 Q-Pistol swap
- F5-F12 configurable weapon slots
- AUTO using the same manual primitives with AUTO values
- INI persistence
- overlay with Save / Reload / Reset Defaults
- clean log
- safe player availability detection

## Features to remove / never reintroduce

- gadget code
- gadget remappers/scanners/producer hooks
- F2 gadget action
- gadget AUTO state
- duplicate manual/AUTO weapon implementations
- duplicate manual/AUTO ammo implementations
- multiple player readiness lanes
- old F4 package state machine
- old Q-Pistol recovery state machine
- historical cumulative state machines copied only for compatibility
- heavy runtime scans unless a current native primitive absolutely requires one

## Primitive audit status

The October-2026 primitive audit is now complete enough to start fresh source implementation.

See `docs/PRIMITIVE_AUDIT_OCT2026.md`.

Locked results:

- `ResolvePlayer()`: current resolver/registry/playerId path mapped.
- `ToggleLicenseToKill()`: current DL-based site mapped and gameplay-validated.
- `GiveWeapon()`: generic pair-clone + current native trigger mapped and gameplay-validated.
- `AddAmmo()`: current native `AddFirearmAmmunitionToPlayer` event path mapped.
- `ApplyWeaponLoadout()`: thin shared caller of `GiveWeapon()`.
- `SwapQPistol()`: thin shared caller of `GiveWeapon()`.

## Implementation rule

The next implementation must be a **fresh minimal core**.

Use U74 only as a behavioral/reference library to identify native primitives.

Do not continue patching U80-U85.

Before the next numbered build:

1. map `ResolvePlayer()` to the October-2026 executable;
2. map `ToggleLicenseToKill()`;
3. map the native weapon primitive needed by `GiveWeapon()`;
4. map the current ammo primitive needed by `AddAmmo()`;
5. determine the smallest safe `SwapQPistol()`;
6. implement `ApplyWeaponLoadout()` only as a thin caller of `GiveWeapon()`;
7. implement AUTO only as a caller of the same manual primitives;
8. add the overlay/config layer only after the gameplay primitives are stable.

## Documentation discipline

After every meaningful test or accepted/rejected build update this file with:

- current canonical/reference base;
- latest verdict;
- next exact step.

Detailed archaeology stays in `docs/HISTORY.md` and `docs/archive/`.
