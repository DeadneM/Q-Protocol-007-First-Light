> [!NOTE]
> **Historical notebook only.** The active architecture was reset in October 2026.  
> Read `/PROJECT_STATE.md` and `/docs/STATUS.md` before using anything in this file.  
> Gadget-era findings below are preserved for research and must not be treated as current product direction.

## 2026-10-04 — Fresh Core A17 optional One/Two-Handed slots

User-supplied `QProtocol.ini` was used as the authority for weapon validation
status updates only. Personal Auto enable/loadout choices were not promoted to
public defaults.

Promoted to Validated:
- AssaultRiflePirate
- SocomPistol
- LightPistolLargeMag
- ShotgunCompact
- ShotgunCompactOneHanded
- AssaultRifleNonLethal
- SMGNonLethal
- ServicePistol

Loadout UI now allows `Off` for QPistol, OneHanded and TwoHanded. `Off` is
stored as `None`, which the existing core already resolves as RID 0.

Public Auto defaults are now QPistolSilenced + Off + Off. Reset Defaults was
also corrected to keep Auto disabled and restore the same Off/Off profile.

No gameplay primitive or RID mapping changed.

## 2026-10-04 — Fresh Core A16 / v0.9.0 release

A16 is a release-only polish step over the audited A15 line.

Functional change:
- `g_showExperimentalLoadout` default changed from `true` to `false`.

Result:
- Experimental entries remain in WeaponCatalog and Weapons UI;
- they no longer appear in Manual/Auto loadout selectors by default;
- the existing checkbox can still expose them for testing.

No gameplay primitive, RID mapping, role, validation status, ammo path, hook or
500 ms stabilization logic changed.

The build workflow now publishes `Q-Protocol_v0.9.0.zip` as GitHub Release
`v0.9.0` after the catalogue audit and compilation pass.

## 2026-10-04 — Fresh Core A15 catalogue audit

Full A13/A14 audit result:

- no gameplay primitive changed;
- no duplicate WeaponCatalog RID was present;
- A14 did unnecessarily rename the existing `ShotgunCompact` alias;
- A15 restores `ShotgunCompact=01833561121578C4`;
- `ShotgunCompactOneHanded=018DCB210A8B048B` remains a separate OneHanded entry;
- `AssassinRifle=0142DF24DDF6819F` remains a separate TwoHanded entry.

A permanent CI validator was added so future builds fail on duplicate RIDs,
missing role/status rows, Weapon/Runtime overlap, stale-RID reuse, or broken
Manual/Auto/hotkey aliases.

## 2026-10-04 — Fresh Core A14 completed A11 firearm catalogue

Final in-game identifications from the A11 runtime sweep:
- ShotgunCompactTwoHanded `01833561121578C4`
- ShotgunCompactOneHanded `018DCB210A8B048B`
- AssassinRifle `0142DF24DDF6819F`

The compact shotgun variants are now separated by role, and AssassinRifle is
added as TwoHanded. All three are Experimental pending explicit gameplay
validation.

Every RID observed in the A11 41-graph sweep is now accounted for.

No A5 gameplay primitive changed.

## 2026-10-04 — Fresh Core A13 firearm RID remap

A Bond-Hashes audit was run against all 41 RIDs discovered by A11.

Ten stale firearm aliases now point at their current October TEMP RIDs, and one
new current firearm (ShotgunCompact) was added to WeaponCatalog.

Those remapped/new entries start as Experimental and are visible in Manual/Auto
selectors by default. Old RIDs remain preserved under [WeaponPreviousRid].

Brick and Vase were classified as throwables. Two A11 RIDs remain unnamed:
- 0142DF24DDF6819F
- 018DCB210A8B048B

No A5 gameplay primitive changed.

## 2026-10-04 — Fresh Core A12 runtime item catalogue

A12 continues directly from A11 without changing gameplay primitives.

A11's runtime scan proved that the ItemEntry/Spawner set includes non-firearm
items. A12 therefore separates runtime identities from WeaponCatalog.

Confirmed runtime identities added:
- MissilePen `01400A15903C9985`
- Laser `01453F3961FC0BB7`
- BlastDevice `015314707AE716BF`
- ShockWave `011B83C48DAC20CA`
- SmokePellets `01BA24E28342EA32`
- Hack `019CF34A2C59C76F`
- Dartgun `01C315FC8C1AEF95`
- GrenadeFlashNPC `017D301CA6D6BF4E`

The remaining A11 runtime RIDs stay Uncatalogued. No guessed identities are
promoted into the catalogue.


## October 2026 architecture reset

After the October 2026 game update, cumulative U80-U85 rebases were rejected.

The project direction was simplified to four shared primitives:

```text
ResolvePlayer()
ToggleLicenseToKill()
GiveWeapon(player, weapon)
AddAmmo(player, profile)
```

F1 toggles LTK, F2 is free, F3 adds ammo, F4-F12 give configured weapons, and AUTO calls the same manual primitives with AUTO values.

Gadget functionality is removed from Q Protocol because the updated game now carries gadgets across missions natively.

The remainder of this file is retained as historical development evidence.

---

# Development history and current findings

This file is a compact reconstruction of the important Q Protocol gadget/weapon lineage. Rejected branches are retained so they are not accidentally reintroduced.

## Weapon architecture

The validated weapon path uses game-native spawning. Manual F4 and AUTO share the same runtime/player readiness infrastructure, while F5–F12 use validated pair-clone firearm spawning.

Important validated Q-Pistol RIDs:

- Silenced Q-Pistol: `019973508A5E327B`
- Unsilenced Q-Pistol: `01BE2C45AA55200D`

Validated pair-clone probes include:

- LightPistolNonLethal: `016886A4B599391C`
- Taser: `01702FE09FEA3596`
- ARExotic: `01587BEED983569A`

The weapon AUTO path is important to U49 because it provides an already-proven point where the current gameplay frame is ready to deliver equipment.

## Gadget producer and record format

The native gadget route reaches the retail producer at:

- EXE + `0x1343F80`
- observed retail caller return address: `0x1416C8B0A`

Validated 24-byte record layout:

```text
+0x00 dword player/context id
+0x04 dword numeric slot
+0x08 qword 0xFFFFFFFFFFFFFFFF
+0x10 dword runtime item token 0x0A01000X
+0x14 dword state, commonly 1 or 2
```

The gadget identity used by the producer is the runtime token at `+0x10`, not the qword at `+0x08`.

## Direction mapping

Validated numeric slot directions:

```text
0 = Left
1 = Up
2 = Down
3 = Right
```

Desired quartet:

```text
Up    = Dartgun
Right = Quick Hack
Down  = SmokePellets
Left  = MissilePen
```

## U30

U30 validated the four-slot remap in game.

The U30/U31 remapper learns live tokens from vanilla retail assignments, then remaps Q Protocol's placeholder records. This works in levels where the vanilla quartet lines up with the positional learning assumptions.

## U31 / U33

U31 integrated the U30 remapper into Q Protocol.

U33 is the canonical fallback. Its ASI is functionally the U31 gadget engine with AUTO gadget disabled through the INI so the old direct-writer path does not compete with the validated manual path.

Canonical U33 ASI SHA-256:

`a0ac354d7c7f2c96ceae2fa3da6cadc0756d325d9fb796a4da2083a223f10dce`

## Cross-level limitation

The U30/U31 remapper learns by vanilla **slot position**, not by gadget identity. In a different level, vanilla Left may be ShockWave instead of MissilePen, so the same positional assumptions can produce the wrong gadget identities.

## U35–U39 diagnostics

These diagnostics isolated the identity problem.

U39 proved that desired gadgets can be resolved by stable runtime fingerprints and that MissilePen can have a valid runtime token even when it is not one of the four currently equipped vanilla gadgets.

Stable instance RIDs:

- MissilePen: `01400A15903C9985`
- SmokePellets: `01BA24E28342EA32`
- Quick Hack: `019CF34A2C59C76F`
- Dartgun: `01C315FC8C1AEF95`

U39 is a research milestone, not a gameplay base.

## Rejected U40–U43 branch

U40–U43 attempted to merge identity resolution into new producer hooks / bridges. These builds either crashed, suppressed output, or added too many moving parts. They must not be used as foundations.

## Rejected U44–U45 branch

These attempted to introduce Manual/Auto gadget profiles with an extra wrapper layer. The wrapper diverged from the validated U33 path, and the first implementation also mishandled internal RID representation. Rejected.

## Rejected U46–U48 branch

U46 called the F2 wrapper from a separate AUTO point but could run before the U30/U31 remapper had learned its live tokens.

U47 retried based on the wrong readiness signal and still mixed the legacy direct writer with the native producer.

U48 introduced another dedicated readiness/helper chain. It still did not solve AUTO and violated the simplification goal.

## U49

U49 follows the user-directed simplification:

```text
AUTO weapons succeed
        ↓
call exact existing F2 wrapper
```

The patch is applied only on the successful AUTO weapon path, inside the same gameplay frame used by the functioning manual systems.

No new gadget writer, producer hook, timer, or independent readiness detector is added.

First gameplay feedback: **appears to work**.

## Next work

1. Validate U49 across multiple levels and respawn/player replacement.
2. Confirm manual F2 remains unchanged.
3. Promote U49 only after those tests.
4. Then separate MANUAL/AUTO gadget configuration if still desired.
5. Finally address cross-level gadget identity using U39 findings, while preserving the validated U30/U31 producer path.


## 2026-09-25/26 — game-update rebase and post-update gadget work

### U54

Full post-update rebase rebuilt directly from U49. Restored the existing architecture after the game executable changed.

### U55 / U56

Rejected AUTO gadget timing experiments.

U56 proved that direct F2 at weapon completion is unsafe when the runtime player exists before the new mission gadget loadout is valid.

### U57

Validated safe AUTO gadget lifecycle. AUTO sets a pending latch; the existing F2 path is synthesized only after live gadget loadout + runtime player readiness are both valid. The second-level transition crash was fixed.

U57 remains the canonical stable lifecycle fallback.

### U58

Diagnostic event trace proving selected runtime gadget objects can be recovered from the retail caller and matched to current tokens.

### U59

Identity-based four-target token learning using selected-object fingerprints instead of native slot position.

### U60

Corrected the real target-record order with a four-byte table change:

```text
0 Left
1 Up
2 Down
3 Right
```

### U61

Diagnostic seven-gadget memory census. Proved a non-equipped MissilePen can remain resident with a valid current-session token. Added a read-only scanner section and dedicated scan buffer.

### U62–U65

Rejected resolver/retry experiments. They introduced unnecessary post-F2 state and retry complexity.

### U66

Rejected prebuilt-list experiment. It anchored the bounded scan to the live mission loadout in the wrong allocation region and also gated manual F2 when the list stayed empty.

### U67

Current working integration/test base.

- U60 gameplay mapping retained.
- U61 scanner plumbing reused without the B-key diagnostic flow.
- scan anchor moved to the exact runtime-player-ready object;
- scan window widened to ±512 MiB;
- manual F2 remains independent of scanner readiness;
- AUTO may wait only while the one-shot list scan is running;
- scan results populate the existing U60 four-target identity-token table.

Latest gameplay feedback: all major systems work again, but the gadget wheel remains wrong. The U67 log reports target mask `0x0F`, so the remaining blocker is the name/identity catalogue rather than F2 execution or AUTO lifecycle.

U67 is accepted as the current working test/integration base, while U57 remains the stable canonical fallback until the seven gadget identities are recaptured authoritatively.
