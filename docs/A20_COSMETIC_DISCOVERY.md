# Fresh Core A20 — Cosmetic / NG+ Discovery

## Goal

Implement a future runtime-only cosmetic unlock layer:

- Unlock All Outfits
- Unlock All Weapon Skins
- Unlock All Gadget Skins
- Unlock All Cosmetics

without requiring challenge completion and without permanently editing the save.

## Evidence from the current executable

The October executable contains reflected/runtime names for:

- `$knt.online.unlockables`
- `knt.online.unlockable.acquire`
- `$knt.loadout.firearmSkins`
- `$knt.loadout.gadgetSkins`
- `$knt.outfits.outfits`
- `knt.gameManager.setOutfit`
- `knt.gameManager.setItemSkin`
- `JSONTemplate.SSkinCollectibleData`
- `JSONTemplate.SOutfitData`
- `JSONTemplate.SUnlockableStateData`

The unlock-state JSON template exposes fields including:

- `CanUnlock`
- `GrantedItems`
- `UnlockableUnlockState`
- `RequiredXP`
- `HasEnoughXP`

## A20 method

A20 does not unlock anything.

It hooks the current-build runtime handlers associated with the cosmetic lists
and online unlockables. After the original game function returns, A20 logs a
bounded snapshot of the object and readable referenced ASCII strings.

The hooks are protected by exact function preimages and are skipped individually
on mismatch.

## Test

Open Customisation / TacSim and browse outfits, weapon skins and gadget skins.
Then send `QProtocol.log`.

The useful lines begin with:

```text
[COSDISC]
```

## Safety

A20:
- does not call `knt.online.unlockable.acquire`;
- does not write `data.save` or `index.save`;
- does not modify challenge completion;
- does not modify unlock-state fields;
- preserves all A19/A5 gameplay behavior.
