# Technical status

## Active state

- Behavioral reference: **U74** on the pre-October-2026 executable.
- Current post-update canonical build: **none**.
- Rejected compatibility branches: U80, U81, U81B, U82, U82A, U83, U84, U85.
- Gadgets: removed.
- Overlay/config model: locked.
- Source of truth: `/PROJECT_STATE.md`.

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

Overlay toggle: **Insert**.

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

## AUTO

AUTO waits for the real player, then calls:

```text
ApplyWeaponLoadout(player, AutoLoadout)
AddAmmo(player, AutoAmmo)
```

When the player disappears or changes, AUTO resets.

No separate AUTO weapon/ammo engine is allowed.

## Overlay

F1-F12 are configurable slots:

```text
Key -> Action -> contextual parameters
```

Actions:

```text
None
LicenseToKill
Ammo
ManualLoadout
SwapQPistol
Weapon
```

Overlay controls:

- Save
- Reload
- Reset Defaults

INI is the only persisted configuration source.

## Next exact step

Primitive audit against the October-2026 executable:

1. `ResolvePlayer()`
2. `ToggleLicenseToKill()`
3. native weapon path for `GiveWeapon()`
4. native ammo path for `AddAmmo()`
5. minimal `SwapQPistol()`

Then implement:

- `ApplyWeaponLoadout()` as a thin `GiveWeapon()` caller;
- AUTO as a thin caller of the same manual primitives;
- overlay only after gameplay core validation.

No new numbered ASI before this audit is complete.
