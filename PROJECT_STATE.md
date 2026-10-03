# Q Protocol — Project State

## Fresh Core A9 Arsenal overlay candidate

- Build: `Q-Protocol_FreshCore_A9_ArsenalOverlay_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**.
- Renderer discovery:
  - no Kiero;
  - no late-only DXGI factory dependence;
  - temporary D3D12/DXGI probe objects provide shared runtime method addresses;
  - MinHook intercepts Present, ResizeBuffers and ExecuteCommandLists globally;
  - first real DIRECT game queue is captured;
  - ImGui initializes only on a same-process/same-device game swapchain.
- Adds full Loadout / Weapons / Hotkeys UI.
- Weapons tab:
  - search;
  - role;
  - RID;
  - editable Status = Validated / Not Working / Experimental;
  - one gameplay action: Spawn Weapon.
- Spawn Weapon crosses to the A5 WorkerThread through an atomic RID mailbox and calls existing QueueWeapon().
- Known prior failures are initialized as Not Working:
  - AgencyFocusGun;
  - SocomPistol;
  - BurstPistol.
- ASI/ZIP hashes: pending workflow build.


## Fresh Core A8 direct DXGI renderer candidate

- Build: `Q-Protocol_FreshCore_A8_DirectDXGI_Renderer_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **A5**, verified by `docs/A5_A7B_GAMEPLAY_AUDIT.md`.
- A8 removes Kiero from the overlay path.
- It hooks DXGI factory swapchain creation directly, captures the game's real D3D12 command queue, then hooks the real Present/ResizeBuffers path.
- First A8 UI is intentionally renderer-only: a compact in-game status panel.
- Full Loadout / Weapons / Hotkeys UI returns only after this renderer is validated.
- A5 gameplay remains fail-open if the renderer fails.
- ASI SHA-256: `70418bf41a06340c603cc003fcfef952b7c2682f45f8da5eeb511b70dca35702`
- ZIP SHA-256: `8091800624f097f20c3818269a542b9db4e3487ae2b43b8663c67649b596baec`
- GitHub Actions run: `37134257650` (PASS).


## Fresh Core A7B fail-open DX12 overlay candidate

- Build: `Q-Protocol_FreshCore_A7B_DX12_FailOpen_TEST.zip`
- Status: **test candidate, not canonical**
- Base: **validated Fresh Core A5 gameplay core**.
- Keeps the true in-game DX12 / Dear ImGui approach.
- Fixes A7's critical fail-open issue:
  - manual hotkeys are blocked only after ImGui is genuinely ready and visible;
  - renderer failure never disables F1-F12.
- Adds per-backbuffer DX12 fences before reusing command allocators.
- Waits overlay GPU work before releasing DX12 resources.
- Captures the first DIRECT command queue, then retires the high-frequency ExecuteCommandLists hook.
- Adds explicit overlay stage diagnostics to QProtocol.log.
- Full role/validation firearm catalogue from A7 is retained.
- ASI SHA-256: `0643f137f81bb4e549ee7af7dd4e71f12e41f30bb4b80eb42795292539183744`
- ZIP SHA-256: `6638a0464b2030781fe974e05897e4151d0bbdd2bd6382f63908765b4e1eb1c3`
- GitHub Actions run: `37071521536` (PASS).

## Fresh Core A7 REJECTED

- A7 is **REJECTED** after user report: "ne marche plus".
- Do not use A7 as a base.
- Primary architectural faults:
  - logical overlay visibility could suppress gameplay before renderer readiness;
  - DX12 command allocator reuse had no explicit fence synchronization.
- A5 remains the gameplay canonical base.

## Fresh Core A7 DX12 ImGui overlay + firearm catalog candidate

- Build: `Q-Protocol_FreshCore_A7_DX12_ImGui_WeaponCatalog_TEST.zip`
- Status: **REJECTED**
- Historical failed candidate only.
- Retired the visible Win32 A6/A6B configuration window.
- Uses a true in-game DX12 Dear ImGui overlay hooked through the game's swap chain.
- Insert remains the overlay toggle.
- Adds Loadout / Weapons / Hotkeys tabs.
- Full known firearm catalogue is retained and classified with:
  - `[WeaponRole]` = QPistol / OneHanded / TwoHanded;
  - `[WeaponValidation]` = Validated / Experimental.
- Loadout selectors show validated weapons by default; experimental entries are opt-in.
- A7 does **not** alter GiveWeapon/AddAmmo gameplay primitives.
- A6B log proves AUTO queues OneHanded correctly; observed failures come from source graph resolution for specific experimental RIDs.
- ASI SHA-256: `7cd5e56a03bdffecb9ce894661ff848459ffb56453810aeb8b7c0e4eaa96c096`
- ZIP SHA-256: `94f4d624f1f2e0438639bf50b607d9bfb3571492389a7472e00e3cfeee1b6b3c`
- GitHub Actions run: `37063281447` (PASS).


> **Primary source of truth.**
> Read this file first when resuming Q Protocol in a new conversation.
> If older README/history notes conflict with this file, this file wins.

## Fresh Core A6B clean overlay test candidate

- Build: `Q-Protocol_FreshCore_A6B_CleanOverlay_TEST.zip`
- Status: **test candidate, not canonical**
- Base: **validated Fresh Core A5**.
- Replaces the cluttered A6 layout with two compact panels: Manual and Automatic.
- F2 is fixed to ManualAmmo and F3 to ManualLoadout.
- Removes visible Profile selectors and debug-only runtime clutter.
- Keeps Insert, Save, Reload, Defaults, player-ready status, Manual editors and AUTO editors.
- A5 gameplay primitives are unchanged.
- ASI SHA-256: `27968278487005b5e525bea73839b9bdcdecdbdf25071e150926325998a5b70f`
- ZIP SHA-256: `49b12a69c06af46e9878a38b8dc3058fd81315e48e890364e4f77994e55e8d90`
- GitHub Actions run: `37042477876` (PASS).

## Fresh Core A6 Insert overlay test candidate

- Build: `Q-Protocol_FreshCore_A6_InsertOverlay_TEST.zip`
- Status: **test candidate, not canonical**
- Base: **validated Fresh Core A5**.
- Adds a renderer-independent native Win32 overlay toggled by Insert.
- Overlay is an INI frontend only; A5 gameplay primitives are unchanged.
- Editable in A6:
  - AUTO Enabled;
  - F2 Profile Manual/Auto;
  - F3 Profile Manual/Auto;
  - ManualLoadout + ManualAmmo;
  - AutoLoadout + AutoAmmo;
  - Save / Reload / Reset Defaults;
  - runtime READY/AUTO/queue/Q-Pistol status.
- `Profile=` is now functional for F2/F3.
- F1-F12 are suppressed while the overlay is visible.
- ASI SHA-256: `9a980b70bdaceb6b1b8c4f8874892c2ff36aded3594e44ad82e9e8dc1a134a4a`
- ZIP SHA-256: `fc12b734426e03c19cc2e3cb7a0ad8d28f31a33afe595892ad2bb0d425118dc2`
- GitHub Actions run: `37014654545` (PASS).

## Fresh Core A5 validated canonical base

- Build: `Q-Protocol_FreshCore_A5_AUTO_SharedPrimitives_TEST.zip`
- Status: **VALIDATED / current canonical base**
- User validation: **PASS on 2026-10-02**.
- Base: **validated Fresh Core A4**.
- AUTO now uses the exact same primitives as manual:
  - `QueueLoadout(AutoLoadout)` -> shared GiveWeapon queue;
  - `QueueAmmoProfile(AutoAmmo)` -> shared native AddAmmo event 0x1DE.
- No duplicate AUTO weapon engine or AUTO ammo writer exists.
- AUTO is configurable with `[Auto] Enabled`, `[AutoLoadout]` and `[AutoAmmo]`.
- AUTO fires once per READY cycle and does not re-arm on same-player loadout pointer refreshes caused by GiveWeapon.
- F1-F12 manual behavior is preserved.
- Overlay remains inactive.
- ASI SHA-256: `ca7c2700601524148bf85766860007e4b35b059debe591aea4ef3ae378329642`
- ZIP SHA-256: `7bb75fd1c351cc6e833b6da17e1e3f001e34b5df750b28d3bee3f606d06fb21b`
- GitHub Actions run: `37003612379` (PASS).

## Fresh Core A4 validated base

- Build: `Q-Protocol_FreshCore_A4_F2_NativeAddAmmo_TEST.zip`
- Status: **VALIDATED / current canonical base**
- User feedback: **positive / accepted on 2026-10-02**
- Base: **validated Fresh Core A3D**.
- Adds only F2 manual reserve-ammo refill through the audited October gameplay primitive `AddFirearmAmmunitionToPlayer`.
- Does **not** resurrect the rejected U80 low-level 24-byte `SetFirearmAmmo` path.
- Native AddAmmo mapping:
  - owner: `*(EXE+0x064576E0)`
  - lock: `owner+0x238E0`
  - staging/input vector: `owner+0x20B70`
  - input pool/context: `owner+0x20AD0`
  - 12-byte insert helper: `EXE+0x00116170`
  - publish helper: `EXE+0x012A8FC0`
  - event: `0x1DE`
- Input record: `{ uint32 playerId, uint32 amount, uint32 firearmClass }`.
- Confirmed classes only: 0 Q-Pistol, 1 SMG/MachinePistol, 2 AR, 5 Shotgun, 6 Sniper/Marksman, 7 HeavyPistol50Cal.
- F2 uses `[ManualAmmo]` and quantities are additive reserve ammo.
- F1/F3/F4/F5-F12 are preserved from A3D.
- AUTO and overlay remain inactive.
- ASI SHA-256: `84ad44faf7f024dc2430803591d17e99ec4c7ffda1a6cdbd64da4dbce0ca2ac4`
- ZIP SHA-256: `a4f0a912c315036da30070e5058fe3c6504a27a3f4f726cd4a340798ec741ecf`
- GitHub Actions run: `37002460495` (PASS).

## Fresh Core A3D validated base

- Build: `Q-Protocol_FreshCore_A3D_TypedLoadout_500msDelay_TEST.zip`
- Status: **VALIDATED / current canonical base**
- User validation: **PASS on 2026-10-02**.
- Base: validated Fresh Core A2.
- A3D adds one shared 500 ms stabilization delay after each GiveWeapon completion before the next queued weapon begins.
- Typed F3 loadout works with exactly:
  - QPistol
  - OneHanded
  - TwoHanded
- F1/F4/F5-F12 remain working.
- ASI SHA-256: `8294f03808fc061faa416cb023c5441d65fdeb4d241ae752e6c3e11fb16a42df`
- ZIP SHA-256: `7a06931d2d47593b00a0ae45d9be20c8ffe9c4006d28cfb09c772680a95c15c6`

## Fresh Core A3C rejected

- Build: `Q-Protocol_FreshCore_A3C_TypedLoadout_QueuePreserve_TEST.zip`
- Status: **REJECTED**
- Result: still only Q-Pistol retained.
- Log proved:
  - all three roles queued;
  - all three GiveWeapon cycles reached COMPLETE;
  - donor restored after each.
- Therefore the remaining issue is post-spawn loadout stabilization, not queue preservation.

## Fresh Core A3B rejected

- Build: `Q-Protocol_FreshCore_A3B_TypedManualLoadout_F3_TEST.zip`
- Status: **REJECTED**
- Result: F3 gave only the Q-Pistol.
- Cause identified in fresh-core logic: the remaining OneHanded and TwoHanded requests were cleared after the first loadout pointer refresh.
- A2 remains canonical until A3C is validated.

## Fresh Core A3 rejected

- Build: `Q-Protocol_FreshCore_A3_ManualLoadout_F3_TEST.zip`
- Status: **REJECTED BEFORE VALIDATION**
- Reason: wrong loadout model. It treated the player loadout as an arbitrary 8-weapon list.
- Correct game model is exactly three typed roles:
  - Q-Pistol: one configured Q-Pistol variant;
  - One-Handed: one one-handed firearm;
  - Two-Handed: one two-handed firearm.
- A3 must never become a base. Fresh Core A2 remains canonical until corrected A3B is validated.

## Fresh Core A2 validated base

- Build: `Q-Protocol_FreshCore_A2_GiveWeapon_F4-F12_TEST.zip`
- Status: **VALIDATED / current fresh-core canonical base**
- Inherits:
  - A1 bootstrap;
  - shared ResolvePlayer();
  - F1 License To Kill, already validated.
- Adds:
  - one shared GiveWeapon() path;
  - one small shared weapon queue;
  - F4 Q-Pistol swap using GiveWeapon();
  - F5-F12 configurable weapon slots using GiveWeapon();
  - one minimal gameplay-thread hook whose only job is the native Spawn(spawner) call.
- F2, F3, AUTO and overlay remain intentionally inactive.
- ASI SHA-256: `b6ff854776439245061d4ad2b1c03ab16b2612a313407bd7876e6a847a9c0a3d`
- ZIP SHA-256: `6f8eac37caa8fab850b4d0d36001aec6735eddbcddf126cb8395685e8b0a181e`
- Validation result: PASS.
- User confirmed:
  - game remains stable in playable mission;
  - F1 still works;
  - F4 Q-Pistol swap works;
  - F5-F12 configured weapons work.
- This validates the new shared GiveWeapon() architecture and the minimal gameplay-thread spawn hook.

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
- Current post-update canonical build: **Fresh Core A5**.
- A5 validates:
  - clean ASI bootstrap;
  - shared ResolvePlayer();
  - F1 License To Kill;
  - F2 native AddAmmo;
  - one shared GiveWeapon() path;
  - F3 typed three-role ManualLoadout;
  - 500 ms inter-weapon stabilization;
  - F4 Q-Pistol swap;
  - F5-F12 configured weapon slots;
  - AUTO using the same shared GiveWeapon/AddAmmo primitives;
  - configurable AutoLoadout + AutoAmmo;
  - one-shot AUTO behavior per READY cycle;
  - minimal gameplay-thread native spawn/ammo dispatch hook.
- Current test candidate: **Fresh Core A9**, testing the runtime D3D12 Arsenal overlay while preserving A5 gameplay.
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

The game loadout is exactly three typed weapon roles:

```text
QPistol   = one Q-Pistol variant
OneHanded = one one-handed firearm
TwoHanded = one two-handed firearm
```

The user chooses those three ManualLoadout values plus the reserve-ammo quantities in `ManualAmmo`.

Default behavior:

```text
F2 -> AddAmmo(player, ManualAmmo)
F3 -> ApplyWeaponLoadout(player, ManualLoadout)
```

### AUTO profile

AUTO uses the exact same three typed roles:

```text
QPistol
OneHanded
TwoHanded
```

The user chooses those three AutoLoadout values independently plus the reserve-ammo quantities in `AutoAmmo`.

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
