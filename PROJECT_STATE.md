# Q Protocol — Project State

> **This file is the primary source of truth for the current project state.**
> Read this file first when resuming Q Protocol in a new conversation.
> If older README/history notes conflict with this file, this file wins for current architecture and next steps.

## Current status

- Historical validated gameplay base: **U74** on the pre-October-2026 game executable.
- Current game executable: October 2026 update.
- Current compatibility status: **not yet validated**.
- U80, U81, U81B, U82, U82A, U83, U84 and U85 are **rejected diagnostic/rebase branches**.
- **Never use U80-U85 as a new canonical base.**
- U74 remains the behavioral reference for extracting validated primitives only.

## Product direction

Q Protocol is being simplified deliberately.

The old accumulated architecture is not to be ported wholesale.

### Gadgets

- Gadget support is **removed from Q Protocol**.
- The game now handles gadgets natively across missions.
- Do not add gadget discovery, remapping, slot forcing, gadget scanners, gadget producer hooks, gadget AUTO logic, or gadget hotkeys.
- **F2 is free.**

## Required architecture

The mod should have one simple shared core.

### 1. Player availability

Use one function:

```text
ResolvePlayer()
```

If no usable player exists, manual weapon/ammo actions do nothing safely.

AUTO watches this same player availability.

When the player becomes available and AUTO has not run for that player/session:

```text
ApplyAutoProfile(player)
```

When the player disappears or changes:

```text
AutoDone = false
```

No separate AUTO readiness architecture is allowed unless proven strictly necessary.

### 2. License To Kill

**F1 = Toggle License To Kill ON/OFF**

This feature is independent from AUTO, weapons and ammo.

One function:

```text
ToggleLicenseToKill()
```

### 3. Ammo

**F3 = add/refill configured ammo**

One function:

```text
AddAmmo(player, profile)
```

Manual F3 uses the manual ammo values.

AUTO calls the exact same `AddAmmo()` function with the AUTO ammo values.

Do not maintain separate manual and AUTO ammo implementations.

The October 2026 game update changed the native ammo backend. The old U74 ammo record must not be copied blindly. Re-audit and implement the current native format cleanly before enabling ammo again.

### 4. Weapons

**F4 through F12 = give the configured weapon if the player is available**

One function:

```text
GiveWeapon(player, weapon)
```

All weapon hotkeys call the same function.

AUTO calls the exact same `GiveWeapon()` function with values from the AUTO profile.

Do not keep separate:
- AUTO GiveWeapon
- manual GiveWeapon
- special F4 package logic
- special Q-Pistol spawn path
- OneHanded/TwoHanded state machines

A small queue/busy flag is allowed only if the native weapon spawner is asynchronous and requires serialization.

## AUTO contract

AUTO must remain conceptually this simple:

```text
player = ResolvePlayer()

if no player:
    AutoDone = false
    return

if player changed:
    AutoDone = false

if not AutoDone:
    apply the same manual primitives using AUTO values
    AutoDone = true
```

AUTO must not duplicate manual functionality.

## Features to keep

- F1: License To Kill toggle
- F3: ammo refill/add
- F4-F12: configurable weapon hotkeys
- AUTO: same functions as manual, with AUTO values
- INI persistence/configuration
- clean logging
- safe player availability detection

## Features to remove / not reintroduce

- gadget code
- F2 gadget action
- gadget remappers
- gadget runtime scanners
- gadget producer hooks
- AUTO gadget state
- duplicate manual/AUTO implementations
- multiple player readiness lanes
- Q-Pistol-specific state machine if generic GiveWeapon can handle it
- recovery spawn paths unless a current native requirement proves them necessary
- old cumulative state machines carried forward merely for compatibility

## Implementation rule

The next implementation must be a **fresh minimal core**.

Use U74 only as a reference library to identify validated native primitives:
- player resolution
- License To Kill
- weapon giving
- ammo handling

Do **not** continue patching U80-U85.

Before producing a new test build:
1. identify the current October-2026 native equivalent for each required primitive;
2. document its inputs, outputs, state dependencies and exact executable references;
3. implement the smallest possible shared wrapper;
4. ensure manual and AUTO call the same wrapper;
5. only then build the next candidate.

## Next exact step

**Audit and extract the four primitives from U74 and map them to the October-2026 executable:**

1. `ResolvePlayer()`
2. `ToggleLicenseToKill()`
3. `GiveWeapon()`
4. `AddAmmo()`

Then build a new minimal ASI around those primitives.

No new numbered compatibility build should be created before this primitive audit is complete.

## Documentation discipline

After every meaningful test or accepted/rejected build, update this file with only:
- current canonical/reference base;
- latest verdict;
- next exact step.

Keep detailed reverse-engineering history in README/docs.
Keep this file short, current and authoritative.
