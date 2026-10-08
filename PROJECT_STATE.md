# Q Protocol — Project State

## Q Protocol v0.9.3 — Fresh Core A20I Native LTK State Toggle

- Build: `Q-Protocol_v0.9.3.zip`
- Release tag: `v0.9.3`
- Status: **validated public release**
- Keeps the validated A20F direct native ItemEntry/Spawner weapon architecture.
- Adds the **Weapons** overlay tab for configuring Weapon 1-8 / F5-F12.
- Renames the previous weapon catalogue / Spawn Weapon / Runtime Discovery area to **Debug**.
- F1 now observes the game's native License To Kill boolean at runtime and forces the opposite state at press time.
- LTK native instructions and the observation hook are restored on DLL shutdown.
- Manual loadout, AUTO, ammo, Q-Pistol swap, F5-F12 and the 31-weapon catalogue remain unchanged.
- Public defaults remain AUTO disabled with OneHanded/TwoHanded set to None.
- Debug Spawn Weapon is retained as a diagnostic tool; normal-story activation can still be context-sensitive.

## A20J Clean Core audit / next candidate

- Base: **validated v0.9.3 / Fresh Core A20I**.
- Two independent cleanup audits agree on the same dead-code set.
- Goal: remove obsolete donor/pair-clone remnants and unused overlay state only.
- No RID, gameplay primitive, timing, AUTO behavior or public default may change.
- Planned core removals include the unreachable donor restore path, `SafeWrite()`, dead LTK state parser and write-only flags.
- Planned overlay cleanup removes write-only telemetry and stale A19/A5 naming.
- Catalogue Spawn Weapon will reuse the existing shared overlay spawn queue helper.
- Runtime Discovery, Debug tools, WeaponPreviousRid and current native hooks stay.
- Main gameplay-hook uninstallation remains out of scope because it would alter lifecycle behavior.
- Detailed checklist: [`docs/A20J_CLEAN_CORE_AUDIT.md`](docs/A20J_CLEAN_CORE_AUDIT.md).
- Next exact step: build **A20J Clean Core TEST** and compare binary/import behavior against A20I.

## Q Protocol v0.9.2 — Fresh Core A20F Direct Native Weapon Graphs

- Build: `Q-Protocol_v0.9.2.zip`
- Release tag: `v0.9.2`
- Status: **validated public release**
- Updated for the October 7, 2026 game executable.
- Corrected `ZKntPlayerLoadoutEntity` vtable mapping restores player-ready detection.
- Weapon spawning now uses each weapon's own native `ItemEntry/Spawner` graph.
- Removed the obsolete donor pair-clone path from active weapon spawning.
- Validated in-game: F1, F2, F3, F4, F5-F12 and AUTO.
- AUTO remains transactional: Q-Pistol -> OneHanded -> TwoHanded -> Ammo.
- Public defaults remain AUTO disabled with OneHanded/TwoHanded set to None.
- Current 31-weapon catalogue and RIDs remain unchanged from A15/A17/A19.

## A19Q Q-Pistol native bootstrap test

- Public/canonical base remains **v0.9.1 / Fresh Core A19 Audit Hardening**.
- Recovered U84 confirms the older October core kept Q-Pistol on a dedicated
  native ItemEntry/Spawner path while extra firearms used pair-clone GiveWeapon.
- Fresh Core A19 unified Q-Pistol into the donor pair-clone.
- A19Q restores only the dedicated native Q-Pistol path.
- AUTO lifecycle/readiness, F3 semantics, extra-weapon GiveWeapon, AddAmmo,
  RIDs, overlay and 500 ms queue delay remain exactly A19.
- Test ZIP intentionally omits QProtocol.ini.

## Q Protocol v0.9.1 — Fresh Core A19 Audit Hardening release

- Build: `Q-Protocol_v0.9.1.zip`
- Release tag: `v0.9.1`
- Status: **public release**
- Gameplay foundation: **validated Fresh Core A5**.
- Full source/config/workflow/documentation audit completed.
- Fixed:
  - overlay-toggle press leaking into F1-F12 when the menu closes;
  - stale F-key edge states while the overlay is open;
  - ToggleKey latch reset that could double-toggle while the opening key is held;
  - core AUTO fallback 1 -> 0 to match shipped defaults;
  - partial MinHook bootstrap cleanup;
  - stale hardcoded-Insert logs;
  - misleading docs claiming generic F1-F12 Action remapping is already implemented.
- CI expanded to strict RID format, runtime category, ammo/default and previous-RID checks.
- Verified current catalogue: 31 weapons (25 Validated, 6 Experimental), 10 runtime-only items, no RID overlap.
- A5 GiveWeapon/AddAmmo/native gameplay hook logic and all weapon RIDs remain unchanged.
- ASI SHA-256: `3f869c7cc654814d0b4fcfbd7bda9e1770ee4ac23734d5f2857b945aac094236`
- INI SHA-256: `8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022`
- ZIP SHA-256: `cc71e3ce99ccd789a7dd330b975db896febd3f3f71a4dcd7fa035bc31e2e60b4`
- GitHub Actions run: `37233791691` (PASS, no compiler warnings/errors).
- Artifact ID: `11314633070`.
- Source build commit: `85e52d8f32cbecdef8f3e82b30ae0e041a975d25`.

## Fresh Core A18 remappable overlay key candidate

- Build: `Q-Protocol_FreshCore_A18_RemappableOverlayKey_TEST.zip`
- Status: **test candidate, not canonical/public release yet**
- Gameplay foundation: **validated Fresh Core A5**.
- Replaces hardcoded `VK_INSERT` overlay toggle with the existing `[Overlay] ToggleKey` setting.
- Hotkeys tab now supports interactive key capture and persistence.
- Invalid/missing ToggleKey safely falls back to Insert.
- Escape cancels capture; modifier-only and mouse buttons are not accepted.
- A17 Off/None loadout behavior, Auto Off/Off defaults, validation statuses and catalogue audit are preserved.
- No gameplay primitive changed.
- ASI SHA-256: `47fbccccc0295b883e4ad9d86301775455c7f423220409f4f38d805ca9cd2fff`
- ZIP SHA-256: `1b816f3f47e8da43db69a3bf6e073652b5feb573318a5693a00812a548410801`
- GitHub Actions run: `37231849616` (PASS).
- Source build commit: `bf912711128337649e1988f296cefd36f60e6825`.

## Fresh Core A17 optional loadout slots candidate

- Build: `Q-Protocol_FreshCore_A17_OptionalLoadoutSlots_TEST.zip`
- Status: **test candidate, not canonical/public release yet**
- Gameplay foundation: **validated Fresh Core A5**.
- OneHanded and TwoHanded selectors now accept `None (Off)` in both Manual and Auto.
- New public Auto defaults:
  - QPistol = QPistolSilenced
  - OneHanded = None
  - TwoHanded = None
  - Auto.Enabled = 0
- Reset Defaults now matches those public defaults.
- User-supplied INI status promotions merged: AssaultRiflePirate, SocomPistol, LightPistolLargeMag, ShotgunCompact, ShotgunCompactOneHanded, AssaultRifleNonLethal, SMGNonLethal, ServicePistol are now Validated.
- Experimental weapons remain hidden from loadout selectors by default.
- No RID mapping or gameplay primitive changed.
- ASI SHA-256: `34baaa9998b3d79c56cdc7651a2d86b270d9f65b55acd170b2cd821db9618f8e`
- ZIP SHA-256: `0bea70aba80661beca4fefeb6dcb02d1770adeae06b6b39513cb241a1b342f69`
- GitHub Actions run: `37202542327` (PASS).
- Built from main commit: `413c785f3992ef07131acde906ab4b63a4182194`.

## Q Protocol v0.9.0 — Fresh Core A16 release

- Build: `Q-Protocol_v0.9.0.zip`
- Release tag: `v0.9.0`
- Status: **public release**
- Gameplay foundation: **validated Fresh Core A5**.
- A16 is intentionally minimal:
  - Experimental weapons are hidden from Manual/Auto lists by default;
  - the user can opt in with the existing overlay checkbox;
  - no WeaponCatalog mapping changed from A15;
  - no gameplay primitive, hook, ammo path or 500 ms stabilization changed.
- A15 catalogue CI audit remains mandatory before compilation.
- ASI SHA-256: `98717095f40b4e34985afdb3e218420697d5fc2b3f7ec4a700ec094d41280d75`
- ZIP SHA-256: `425f78baa079948dff7b039132c5cbecedbf0e2306baf3d0566126b258270785`
- GitHub Actions run: `37191187144` (PASS).
- Release ID: `402934502`; asset `Q-Protocol_v0.9.0.zip`.
- Release source commit/tag target: `f172fe246172994dab42d68afd147abc94c8faee`.

## Fresh Core A15 audited weapon catalogue candidate

- Build: `Q-Protocol_FreshCore_A15_AuditedWeaponCatalog_TEST.zip`
- Status: **test candidate, not canonical**
- Full A13/A14 audit completed.
- **Correction:** restore the existing alias `ShotgunCompact=01833561121578C4` instead of A14's unnecessary rename to `ShotgunCompactTwoHanded`.
- Keep the genuinely new aliases:
  - `ShotgunCompactOneHanded=018DCB210A8B048B` — OneHanded
  - `AssassinRifle=0142DF24DDF6819F` — TwoHanded
- No duplicate RID exists in WeaponCatalog.
- Gameplay source audit: A13 -> A14 changed only version strings; A5 primitives/hook/timing are untouched.
- Adds a permanent CI catalogue-consistency check for duplicate RIDs, missing roles/statuses, invalid references, Runtime overlap and stale-RID reuse.
- Experimental weapons remain visible in Manual/Auto by default for testing.
- ASI SHA-256: `ada0fce570ec50cf74a3ecc3177a643ca7d37dc698ea0b18d147461631496fb2`
- ZIP SHA-256: `84e7ab3e6a84fdf9ddac39b74e60dd164fe866ea8f3eb83647e96f0316eceedf`
- GitHub Actions run: `37190013537` (PASS).
- Source build commit: `06ec8fd1857f582f5da038621431479c356cc4b4`.

## Fresh Core A14 completed A11 firearm catalogue candidate

- Build: `Q-Protocol_FreshCore_A14_CompletedWeaponCatalog_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**; gameplay primitives unchanged.
- Completes the A11 41-graph runtime catalogue with three final in-game identifications:
  - ShotgunCompactTwoHanded `01833561121578C4` — TwoHanded
  - ShotgunCompactOneHanded `018DCB210A8B048B` — OneHanded
  - AssassinRifle `0142DF24DDF6819F` — TwoHanded
- All three start as Experimental and are visible in Manual/Auto selectors.
- Every RID from the A11 runtime sweep is now accounted for.
- ASI SHA-256: `0d661c1a1c911d2bec873f2c4f3ff175d4d078213725f6795da0cb813334e76a`
- ZIP SHA-256: `b4a2ee0deb3e4fa6f337ef39eab01fbe237a4bacff1a62b66d80476aa3abad34`
- GitHub Actions run: `37189392074` (PASS).
- Source build commit: `799b04030fdb6f1d57d1b6294a2a49f420182a35`.

## Fresh Core A13 October firearm RID remap candidate

- Build: `Q-Protocol_FreshCore_A13_WeaponRIDRemap_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**; gameplay primitives unchanged.
- Derived from A12/A11 runtime discovery and a Bond-Hashes audit of all 41 A11 graph RIDs.
- Updated current RIDs:
  - AgencyFocusGun `0198799A3CC4E437`
  - AssassinHandcannon `017D8BD5B237B333`
  - ShotgunStandard `01BD00B144B74AC0`
  - AssaultRiflePirate `0110285AF7A95A01`
  - SocomPistol `01D73DA578C4F423`
  - LightPistolLargeMag `010ABE3F66032326`
  - AssaultRifleNonLethal `016D12D89A0A658E`
  - SMGNonLethal `018C90273080F785`
  - BurstPistol `015E2DA84660F7D9`
  - ServicePistol `01988D661ADF3BCE`
- New current firearm: ShotgunCompact `01833561121578C4`.
- Remapped/new entries start Experimental and are visible in Manual/Auto selectors by default.
- Previous stale RIDs are retained in `[WeaponPreviousRid]`.
- Brick and Vase are classified as Throwable runtime items.
- Only `0142DF24DDF6819F` and `018DCB210A8B048B` remain unnamed from the A11 runtime set.
- ASI SHA-256: `73695e97036f2428c35e678e958f1c6689869af2daf421e1dbc2c6b834e467e6`
- ZIP SHA-256: `a91ab0c4ac7703a0d4f8b854ae7847f4508d5193c877e397a8d51773141b4949`
- GitHub Actions run: `37186162147` (PASS).
- Source build commit: `2f527c358d6b14a3ab82e5b3d3767621ca59fcd2`.

## Fresh Core A12 runtime item catalogue candidate

- Build: `Q-Protocol_FreshCore_A12_RuntimeCatalog_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**.
- Derived surgically from Fresh Core A11; gameplay primitives are unchanged.
- Preserves the user's supplied A11 INI state, including `Auto.Enabled=0` and the complete weapon validation table.
- Adds a separate `RuntimeCatalog` so non-firearm runtime graphs are named without entering `WeaponCatalog`.
- Identified from A11 evidence:
  - MissilePen `01400A15903C9985` — Gadget
  - Laser `01453F3961FC0BB7` — Gadget
  - BlastDevice `015314707AE716BF` — Gadget
  - ShockWave `011B83C48DAC20CA` — Gadget
  - SmokePellets `01BA24E28342EA32` — Gadget
  - Hack `019CF34A2C59C76F` — Gadget
  - Dartgun `01C315FC8C1AEF95` — Gadget
  - GrenadeFlashNPC `017D301CA6D6BF4E` — Grenade
- Runtime Discovery now shows Item / Type / Current level / Identified state / RID.
- Unknown A11 RIDs remain visible as Uncatalogued; no speculative names are added.
- Existing Spawn Weapon test path remains the A5 QueueWeapon mailbox.
- ASI SHA-256: `85b124a59d1c5f354d0ed50e053920624a7cd881b610270627980d43eb07b117`
- ZIP SHA-256: `d37dcebc3e117f727f0fe187f8ecfb8ab9c3a5c4bb43864802594e9be74467b4`
- GitHub Actions run: `37184389495` (PASS).
- Source build commit: `a54b7f216fbdd4f3cd74bf7a31fb3ad98bea5b63`.

## Fresh Core A11 runtime weapon discovery candidate

- Build: `Q-Protocol_FreshCore_A11_RuntimeWeaponDiscovery_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**.
- Retains A10/A9 DX12 Arsenal renderer.
- Adds Q-Pistol `Off` for both Manual and Automatic loadouts.
  - UI `Off` persists as `QPistol=None`;
  - A5 already parses `None` as RID 0, so no new gameplay implementation exists.
- Preserves the user's latest `Validated / Not Working / Experimental` classifications exactly.
- Not Working / Experimental entries are intentionally retained as rediscovery targets.
- Adds `Weapons -> Runtime Discovery`:
  - current loaded ItemEntry/Spawner graph RIDs;
  - uncatalogued RIDs;
  - current-level vs seen-earlier-this-session state;
  - Spawn Weapon through existing A5 QueueWeapon path.
- Runtime discoveries accumulate across level transitions for the current session.
- QProtocol.log writes each newly observed graph as:
  - `DISCOVERY runtime graph RID=XXXXXXXXXXXXXXXX`
- Goal: recover new October RIDs for legacy failures and detect weapons absent from the catalogue.
- ASI SHA-256: `e555bec0dbd22ef201d93bc7cb34f2bdb478bfc5e44f96ca154b4738bf61e4dd`
- ZIP SHA-256: `cfe919f41b413193674096cf2eed58dd92f7c88dffd9ecd48c6689b300dad078`
- GitHub Actions run: `37145362694` (PASS).


## Fresh Core A10 weapon revalidation + UI fix candidate

- Build: `Q-Protocol_FreshCore_A10_WeaponRevalidation_UIFix_TEST.zip`
- Status: **test candidate, not canonical**
- Gameplay base: **validated Fresh Core A5**.
- Retains the working A9 Arsenal overlay architecture.
- Weapon catalogue status is reset to October-only evidence:
  - Validated = post-update Fresh Core success;
  - Not Working = post-update Fresh Core failure;
  - Experimental = legacy/known RID requiring re-validation.
- October-Validated defaults:
  - QPistolSilenced;
  - QPistolUnsilenced;
  - HeavyPistol50Cal;
  - ARMilitary;
  - Taser;
  - MachinePistolHighRecoil;
  - ShotgunSemiAuto;
  - LightPistolNonLethal;
  - AssaultRifleNonLethal;
  - SMGNonLethal.
- Known current failures:
  - AgencyFocusGun;
  - SocomPistol;
  - BurstPistol.
- Ammo UI rebuilt as one row per class with wider numeric fields and no clipped +/- steppers.
- Overlay maximum target size increased to 980x740.
- ASI SHA-256: `85870b04921b1fa92f11aca2188f0f93ea60a0f7c06e228f10f0746435d8e830`
- ZIP SHA-256: `f7a2092aa09a4ed4c37901d0738bce6e346e0cce03e84e25ac23faece609cd69`
- GitHub Actions run: `37144221679` (PASS).


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
- ASI SHA-256: `9ef3bedec76cfe7dd499288092244a7e87ff26b30bbe2d0b17b9b1227401fa12`
- ZIP SHA-256: `223781380f2bc5d9498deaacda0fe631e3a35b24269e00cb88b40f04149f66e4`
- GitHub Actions run: `37140263601` (PASS).


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
- Current test candidate: **Fresh Core A11**, adding Q-Pistol Off and runtime RID discovery while preserving A5 gameplay.
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
