# Technical status

## Active state

- Behavioral reference: **U74** on the pre-October-2026 executable.
- Current post-update canonical build: **Fresh Core A2**.
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

## Primitive audit

Completed. See [`PRIMITIVE_AUDIT_OCT2026.md`](PRIMITIVE_AUDIT_OCT2026.md).

Key result: the October executable exposes a native `AddFirearmAmmunitionToPlayer` input, so F2/AUTO ammo no longer need the low-level reserve-vector setter.

The generic pair-clone/native weapon trigger used on the October executable is retained only behind one shared `GiveWeapon()`.

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
