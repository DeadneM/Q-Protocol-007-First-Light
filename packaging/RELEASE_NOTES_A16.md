# Q Protocol v0.9.0 — Fresh Core A16

This is the first public release from the audited October Fresh Core line.

## Release change

- Experimental weapons are now **hidden from Manual/Auto loadout lists by default**.
- They remain available in the Weapons catalogue and can be exposed with **Show Experimental weapons in loadout lists**.
- Validated weapons remain available normally.

## Catalogue integrity

This release keeps the A15 audited catalogue:
- no duplicate weapon RIDs;
- no WeaponCatalog / RuntimeCatalog overlap;
- no current weapon RID reuses an archived stale RID;
- complete role/status rows for every weapon;
- Manual, Auto and F4/F5-F12 aliases validated by CI before compilation.

## Gameplay

No gameplay primitive changed in A16. Fresh Core A5 remains the validated gameplay foundation for GiveWeapon, AddAmmo, Manual/Auto loadouts and the native gameplay hook.

## Package

The release ZIP contains exactly:
- `QProtocol.asi`
- `QProtocol.ini`
- `README.txt`
