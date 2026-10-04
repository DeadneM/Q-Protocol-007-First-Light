# Fresh Core A19 — Full Mod Audit

## Scope

Audited the active gameplay core, gameplay trampoline, A18 DX12 overlay, INI
schema, GitHub Actions workflow, packaging, README, STATUS, PROJECT_STATE and
overlay design documentation.

## Verified gameplay invariants

Fresh Core A5 remains the gameplay foundation:

- one shared player resolver;
- one shared GiveWeapon queue;
- one shared native AddAmmo path;
- Manual and AUTO use the same gameplay primitives;
- 500 ms inter-weapon stabilization remains unchanged;
- AUTO remains one-shot per READY cycle;
- player identity changes reset runtime state;
- same-player loadout pointer refresh preserves an active weapon queue;
- LTK writes remain preimage-checked;
- gameplay hook installation remains preimage-checked.

## Catalogue

Current default INI:

- 31 weapon aliases and 31 unique current weapon RIDs;
- 25 Validated, 6 Experimental;
- 2 QPistol, 13 OneHanded, 16 TwoHanded;
- 10 runtime-only item RIDs;
- 10 archived previous firearm RIDs;
- no WeaponCatalog / RuntimeCatalog RID overlap;
- no current weapon reuses a previous stale RID.

## Fixed in A19

### Overlay toggle vs F1-F12
A18 could close the overlay with a remapped F1-F12 key and dispatch that same
press to gameplay. A19 consumes the overlay toggle edge first.

### Stale F-key edges
A18 did not advance F1-F12 edge state while the overlay was open. A19 updates
edge state every loop and gates only action dispatch.

### ToggleKey double-edge
A18 reset the toggle latch to false when UI configuration reloaded. A still-held
opening key could be seen as a second press. A19 latches the real physical state.

### AUTO fallback
The shipped INI and overlay default to Auto.Enabled=0, but the gameplay-core
fallback was 1. A19 changes the core fallback to 0.

### Partial overlay-hook failure
A partial MinHook bootstrap failure could leave already-enabled overlay hooks
alive. A19 uninitializes MinHook immediately on bootstrap failure.

### CI/config integrity
CI now additionally checks exact 16-hex RID format, RuntimeCategory coverage,
previous-RID uniqueness and alias integrity, Auto.Enabled, Overlay.ToggleKey,
and exact ammo key/range validity.

## Documentation correction

The historical overlay design describes generic Key -> Action remapping for
F1-F12. The current runtime does not implement that layer.

Current truth:
- F1-F4 action semantics are fixed;
- F5-F12 are fixed Weapon actions with configurable weapon aliases;
- Action= rows in the INI are descriptive today;
- the overlay invocation key itself is remappable.

## Residual risks intentionally unchanged

- The ASI is designed as a process-lifetime mod. Manual DLL unloading is not a
  supported runtime path and the gameplay trampoline is not redesigned here.
- Executable acceptance uses PE timestamp + SizeOfImage plus per-feature
  preimage/runtime checks. A19 does not add a whole-file hash lock.
- Historical OverlayA8..A18 and U49 material remain for forensic history; the
  workflow explicitly compiles only OverlayA19.cpp.
- Generic F1-F12 Action remapping remains a future feature.

## Conclusion

No duplicate gameplay engine or catalogue corruption was found. The meaningful
audit defects were input-edge handling, fallback consistency, partial overlay
hook cleanup and stale documentation. A19 fixes those without changing validated
gameplay primitives or weapon mappings.
