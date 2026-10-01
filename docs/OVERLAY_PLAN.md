# Q Protocol Overlay Plan

## Goal

Provide a small in-game configuration overlay without creating a second gameplay architecture.

The overlay is only a frontend for the same INI-backed gameplay primitives used by hotkeys and AUTO.

## Default toggle

```text
Insert
```

F1-F12 remain fully available for gameplay bindings.

## Slot model

Every function key is represented as:

```text
Key
  -> Action
  -> contextual parameters
```

Default bindings:

| Key | Action |
|---|---|
| F1 | LicenseToKill |
| F2 | Ammo |
| F3 | ManualLoadout |
| F4 | SwapQPistol |
| F5-F12 | Weapon |

## Supported actions

### None

No action.

### LicenseToKill

Calls:

```text
ToggleLicenseToKill()
```

No extra parameter.

### Ammo

Calls:

```text
AddAmmo(player, profile)
```

Parameters:

- Profile: Manual / Auto

Default F2 profile: Manual.

### ManualLoadout

Calls:

```text
ApplyWeaponLoadout(player, profile)
```

Default F3 profile: Manual.

### SwapQPistol

Calls:

```text
SwapQPistol(player)
```

Parameters:

- Mode A weapon
- Mode B weapon

Defaults:

```text
QPistolSilenced
QPistolUnsilenced
```

### Weapon

Calls:

```text
GiveWeapon(player, weapon)
```

Parameter:

- Weapon alias/RID

## Manual / AUTO profile editors

The overlay must expose two parallel profile editors.

### Manual

```text
ManualLoadout
  Weapon1
  Weapon2
  Weapon3
  ...

ManualAmmo
  QPistol
  SMG
  AssaultRifle
  Shotgun
  Sniper
  HeavyPistol
```

F3 consumes `ManualLoadout`.

F2 consumes `ManualAmmo`.

### AUTO

```text
AutoLoadout
  Weapon1
  Weapon2
  Weapon3
  ...

AutoAmmo
  QPistol
  SMG
  AssaultRifle
  Shotgun
  Sniper
  HeavyPistol
```

AUTO consumes both profiles through the exact same gameplay primitives used manually.

Weapon slots may be set to `None`.

Ammo quantities are editable independently for Manual and AUTO.

## Overlay controls

### Save

Write current overlay settings to `QProtocol.ini`.

### Reload

Discard unsaved UI changes and reload `QProtocol.ini`.

### Reset Defaults

Restore authoritative defaults:

```text
F1 LicenseToKill
F2 Ammo / Manual
F3 ManualLoadout / Manual
F4 SwapQPistol
F5-F12 Weapon

ManualLoadout -> default manual weapon list
ManualAmmo    -> default manual ammo quantities
AutoLoadout   -> default automatic weapon list
AutoAmmo      -> default automatic ammo quantities
```

## Status area

Show only useful runtime state:

- player: READY / NOT READY
- AUTO: WAITING / DONE
- current Q-Pistol mode
- queued weapon count if the native spawner requires serialization
- last action result

Do not turn the overlay into a debugger console.

## Persistence

The INI is the only configuration source.

The overlay reads/writes the same sections used by the gameplay core.

No hidden secondary configuration database.

## Architecture

```text
Input / Overlay
      |
      v
BindingManager
      |
      +--> ResolvePlayer()
      +--> ToggleLicenseToKill()
      +--> AddAmmo()
      +--> ApplyWeaponLoadout()
      +--> SwapQPistol()
      +--> GiveWeapon()

AUTO
      |
      +--> ResolvePlayer()
      +--> ApplyWeaponLoadout(Auto)
      +--> AddAmmo(Auto)
```

Manual hotkeys and AUTO therefore converge on the same gameplay layer.

## Implementation order

1. Build and validate the gameplay primitives.
2. Validate default F1-F12 actions without the overlay.
3. Add INI parser for generic slots.
4. Add overlay rendering/input.
5. Add Save / Reload / Reset Defaults.
6. Only then add optional rebinding from the overlay.

The overlay must not be used to hide instability in the gameplay primitives.
