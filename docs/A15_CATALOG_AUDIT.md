# Fresh Core A15 — Catalogue Audit

## Audit result

A13 and A14 were compared at source and configuration level.

### Gameplay code

No gameplay regression was found.

`OverlayA13.cpp` and `OverlayA14.cpp` differ only in eight version/status
strings. `QProtocol.cpp` changed only its build/scope log strings.

The following are unchanged:
- ResolvePlayer;
- GiveWeapon;
- AddAmmo;
- Manual loadout queue;
- Automatic loadout queue;
- native gameplay spawn hook;
- 500 ms inter-weapon stabilization.

### Catalogue issue found

A13 introduced:

`ShotgunCompact=01833561121578C4`

A14 renamed it to:

`ShotgunCompactTwoHanded=01833561121578C4`

This did not create a duplicate RID, but it was an unnecessary alias rename and
could invalidate an existing configuration that already selected
`ShotgunCompact`.

A15 restores the original alias and keeps only the genuine additions:

| Alias | RID | Role |
|---|---|---|
| ShotgunCompact | 01833561121578C4 | TwoHanded |
| ShotgunCompactOneHanded | 018DCB210A8B048B | OneHanded |
| AssassinRifle | 0142DF24DDF6819F | TwoHanded |

### Consistency checks

The audited catalogue has:
- unique WeaponCatalog aliases;
- unique WeaponCatalog RIDs;
- no WeaponCatalog / RuntimeCatalog RID overlap;
- no current RID reused from WeaponPreviousRid;
- a WeaponRole and WeaponValidation row for every weapon.

A15 also makes those checks permanent in GitHub Actions before compilation.
