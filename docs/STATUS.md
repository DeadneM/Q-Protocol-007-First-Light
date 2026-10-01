# Technical status

## Active state

- **Behavioral reference:** U74 on the pre-October-2026 executable.
- **Current post-update canonical build:** none.
- **Rejected compatibility branches:** U80, U81, U81B, U82, U82A, U83, U84, U85.
- **Gadgets:** removed from active Q Protocol scope.
- **F2:** free.
- **Source of truth:** `/PROJECT_STATE.md`.

## Required minimal core

The next implementation must expose only these shared primitives:

```text
ResolvePlayer()
ToggleLicenseToKill()
GiveWeapon(player, weapon)
AddAmmo(player, profile)
```

### Manual controls

- F1 -> `ToggleLicenseToKill()`
- F3 -> `AddAmmo(player, ManualProfile)`
- F4–F12 -> `GiveWeapon(player, configuredWeapon)`

### AUTO

AUTO waits for a valid player and then calls the **same** gameplay primitives with `[Auto]` values.

No separate AUTO weapon or AUTO ammo implementation should exist.

## October 2026 update

The new executable requires a fresh primitive-level audit.

Do not port U74's cumulative state-machine architecture wholesale.

Known conclusions from the rejected rebase work:

- simple RVA shifting is insufficient;
- short byte signatures can match the wrong function;
- the old ammo backend changed materially and must be re-audited;
- gadget-era sections contain mixed historical code and must not be used as the basis for the new core.

## Next exact step

Audit and map, in this order:

1. `ResolvePlayer()`
2. `ToggleLicenseToKill()`
3. `GiveWeapon()`
4. `AddAmmo()`

For each primitive record:

- old U74 behavior/reference;
- current October-2026 native equivalent;
- inputs and outputs;
- required globals/vtables/offsets;
- asynchronous/state requirements;
- exact validation evidence.

Only after all four are understood should a new numbered ASI candidate be built.
