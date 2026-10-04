# Fresh Core A12 — Runtime Item Catalog

A12 is built directly on Fresh Core A11 and keeps Fresh Core A5 as the validated
gameplay base.

## Why this build exists

A11 discovered 41 live ItemEntry/Spawner RID graphs. The discovery set was
broader than firearms: it also contained gadget instances and at least one
grenade instance.

A12 does not change the scanner. It adds a second catalogue dedicated to these
non-firearm runtime items.

## Identified runtime RIDs

| Alias | RID | Category |
|---|---|---|
| MissilePen | 01400A15903C9985 | Gadget |
| Laser | 01453F3961FC0BB7 | Gadget |
| BlastDevice | 015314707AE716BF | Gadget |
| ShockWave | 011B83C48DAC20CA | Gadget |
| SmokePellets | 01BA24E28342EA32 | Gadget |
| Hack | 019CF34A2C59C76F | Gadget |
| Dartgun | 01C315FC8C1AEF95 | Gadget |
| GrenadeFlashNPC | 017D301CA6D6BF4E | Grenade |

These entries are stored under `[RuntimeCatalog]` and `[RuntimeCategory]`.
They are deliberately not inserted into `[WeaponCatalog]`.

## Unknown A11 runtime RIDs

The other RIDs observed by A11 remain Uncatalogued. A12 does not assign a name
until there is evidence for it.

## UI

`Weapons -> Runtime Discovery` now shows:
- item name;
- type;
- whether the graph is present in the current level;
- Identified or Uncatalogued;
- exact RID;
- Spawn Weapon, unchanged from A11.

## Invariants

- Fresh Core A5 gameplay primitives unchanged.
- Weapon statuses preserved.
- Not Working and Experimental weapons are not deleted.
- User INI state preserved, including Auto.Enabled=0.
- No gadget functionality is reintroduced. The gadget names here are discovery
  metadata only.
