# Q Protocol v0.9.1 — Fresh Core A19 Audit Hardening

This release promotes the fully audited A19 Fresh Core candidate.

## Main changes

- Overlay toggle key is remappable and persisted through `[Overlay] ToggleKey`.
- Remapped overlay keys can safely overlap F1-F12 without triggering gameplay actions while opening or closing the menu.
- F1-F12 edge state is hardened against held-key leakage when the overlay closes.
- OneHanded and TwoHanded loadout slots support `Off / None`.
- Automatic defaults are `QPistolSilenced + Off + Off`.
- Core `Auto.Enabled` fallback now matches the shipped default: disabled.
- Partial DX12/MinHook bootstrap failures clean up immediately.
- CI performs strict weapon/runtime/config integrity checks before compilation.
- Documentation now accurately distinguishes current fixed F1-F12 action semantics from the future generic Action-remap design.

## Catalogue

- 31 firearm aliases
- 25 Validated
- 6 Experimental
- 10 runtime-only items
- no duplicate current weapon RIDs
- no WeaponCatalog / RuntimeCatalog overlap
- no current weapon reuses an archived stale RID

## Gameplay

Fresh Core A5 remains the validated gameplay primitive foundation. A19 does not change the shared GiveWeapon/AddAmmo architecture, weapon mappings, ammo classes, or 500 ms weapon stabilization.

## Package

The release ZIP contains exactly:
- `QProtocol.asi`
- `QProtocol.ini`
- `README.txt`
