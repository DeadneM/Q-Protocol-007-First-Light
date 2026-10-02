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
  QPistol
  OneHanded
  TwoHanded

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
  QPistol
  OneHanded
  TwoHanded

AutoAmmo
  QPistol
  SMG
  AssaultRifle
  Shotgun
  Sniper
  HeavyPistol
```

AUTO consumes both profiles through the exact same gameplay primitives used manually.

The loadout editor preserves the game's three roles:

- QPistol: Q-Pistol variants only;
- OneHanded: one-handed firearms only;
- TwoHanded: two-handed firearms only.

F5-F12 remain standalone weapon hotkeys and are not loadout slots.

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


## A6 implementation status

Fresh Core A6 implements the first usable overlay layer.

Implemented:
- native Win32 renderer-independent overlay;
- Insert toggle;
- status area;
- Manual and AUTO three-role loadout editors;
- Manual and AUTO ammo editors;
- AUTO Enabled;
- live F2/F3 Manual/Auto profile selectors;
- Save / Reload / Reset Defaults;
- WeaponCatalog-driven selectors;
- manual F1-F12 suppression while the overlay is visible.

Not yet implemented:
- generic reassignment of every F1-F12 Action;
- F4 Mode A / Mode B editor;
- F5-F12 weapon editor.

Those remain the next overlay layer after A6 visibility/input/persistence is validated.


## A6B UI simplification

User feedback on A6: the overlay was functional but confusing and overcrowded.

A6B removes UI elements that expose internal plumbing:
- no F2/F3 Profile selectors;
- no AUTO DONE/WAITING debug status;
- no weapon queue status;
- no Q-Pistol next-mode status;
- no duplicate internal Q Protocol title.

A6B uses two compact panels:
- Manual: F2 ammo + F3 three-role loadout;
- Automatic: Enable + AutoAmmo + AutoLoadout.

F2 and F3 are intentionally fixed to Manual values again. This matches the
validated A5 behavior and avoids a configuration concept that was not useful
for normal play.
