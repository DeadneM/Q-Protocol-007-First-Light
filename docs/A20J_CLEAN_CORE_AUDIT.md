# A20J Clean Core Audit

Date: 2026-10-08

Canonical base: **Q Protocol v0.9.3 / Fresh Core A20I**

Purpose: clean dead code and stale historical naming without changing validated gameplay behavior.

## Hard rule

A20J must not change:

- weapon RIDs;
- the 31-weapon catalogue;
- F1/F2/F3/F4/F5-F12 semantics;
- AUTO sequencing;
- native spawn timing/retries;
- player readiness;
- ammo behavior;
- Q-Pistol behavior;
- Runtime Discovery behavior;
- public INI defaults.

## Audit result

Two independent passes over `QProtocol.cpp`, `OverlayA19.cpp`, `Overlay.h`,
`GameplayHook.asm` and `QProtocol.ini` reached the same conclusion.

### Safe to remove

Core:
- `kDonorDisplayRid`;
- `SafeWrite()`;
- `ActiveWeapon::descriptorPatched`;
- `ActiveWeapon::originalDonorRid`;
- `ActiveWeapon::originalDonorTemplate`;
- `ActiveWeapon::donorItem`;
- `ActiveWeapon::directGraph`;
- `ActiveWeapon::seenBusy`;
- unreachable donor restoration branch and its logs;
- `ReadLicenseToKillState()`;
- `LtkState`;
- `g_gameplayHookInstalled`.

Overlay:
- `g_autoDone`;
- `g_queueCount`;
- overlay copy of `g_qpistolNextB`;
- unused corresponding `OverlayPump()` parameters;
- `g_hooksInstalled`.

### Safe rename / simplify

- `donorSpawner` -> `spawner`;
- `BeginWeaponResult::DonorBusy` -> `SpawnerBusy`;
- `RestoreActiveDonor()` -> a direct active-weapon state reset helper;
- `DrawModTab()` -> `DrawWeaponsTab()`;
- `g_modWeaponSlots` -> `g_weaponSlots`;
- old debug catalogue `DrawWeaponsTab()` -> `DrawDebugTab()`;
- remove stale A19/A5 labels from current overlay status text;
- route catalogue Spawn Weapon through existing `QueueOverlaySpawn()` instead of duplicating the same queue logic;
- reduce `OverlayPump()` to the data it actually consumes.

## Keep

- `BuildGraphIndex()`, `ValidateGraph()`, `ResolveGraph()`;
- spawner idle/acceptance/completion observation;
- Native Spawn queue and retry timing;
- Runtime Discovery and RuntimeCatalog/RuntimeCategory;
- WeaponValidation;
- WeaponPreviousRid, because CI uses it as stale-RID regression protection;
- Debug Spawn Weapon, even though Story mode can be context-sensitive;
- the real gameplay Q-Pistol state in `QProtocol.cpp`;
- MinHook/DX12 overlay hooks;
- License To Kill native-state observation hook.

## Important nuance

Removing `SafeWrite()` removes Q Protocol's own use of `WriteProcessMemory`.
Whether the final PE import also disappears must be verified on the compiled A20J binary because third-party linked code may still reference it.

The main gameplay hook is currently installed for process lifetime and is not explicitly restored on shutdown. This is deliberately **out of scope for A20J** because adding an unhook path would be a lifecycle behavior change rather than dead-code cleanup.

## A20J acceptance criteria

A20J is accepted only if:

1. CI catalogue/config audit passes unchanged.
2. Build succeeds with no new compiler errors.
3. F1, F2, F3, F4, F5-F12 and AUTO behave exactly like A20I.
4. Overlay tabs remain Loadout / Weapons / Debug / Hotkeys.
5. Runtime Discovery still works.
6. Public INI and RIDs remain unchanged.
7. PE/import comparison is recorded against A20I.
8. Any Defender change is treated as an observation, not as proof of safety by itself.


## First A20J build result

- Branch: `dev/a20j-clean-core`
- Head: `567282129863899a424136795728e3f9c08e6d32`
- GitHub Actions: run #113 / `37835613207`
- Result: **PASS**
- Config/catalogue validation: **PASS**
- MSVC build: **PASS**
- PE import audit: **PASS**
- `WriteProcessMemory`: **absent**
- `ReadProcessMemory`: present as expected
- `VirtualProtect`: present as expected
- Gameplay validation: **PASS**

Source diff against the documented A20I base is cleanup-dominant:
- `src/QProtocol.cpp`: 112 deletions / 16 additions
- `src/OverlayA19.cpp`: 77 deletions / 30 additions
- `src/Overlay.h`: 5 deletions / 1 addition

The public v0.9.3 release remains untouched while A20J is tested.


## In-game validation result

User verdict: **everything works perfectly**.

Validated from the A20J runtime log:
- executable accepted;
- gameplay hook installed;
- native LTK observation hook installed;
- DX12 overlay reached ImGui-ready state;
- AUTO weapon package completed and AUTO ammo published;
- F3 Manual package completed all three roles;
- F1 toggled native LTK ON -> FORCE OFF and OFF -> FORCE ON;
- F2 ammo published all six firearm classes;
- F5/F6/F7/F10/F11 direct native weapon slots completed successfully;
- Debug Spawn Weapon completed repeated direct native spawns;
- PLAYER NOT READY -> READY transition reset correctly;
- AUTO and Manual loadout worked again after the READY-cycle transition;
- F4 Q-Pistol direct swap remained functional.

A20J is therefore **VALIDATED** and can replace A20I as the canonical cleanup base.
