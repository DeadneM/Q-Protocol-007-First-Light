Q Protocol - Fresh Core A19T TacSim Compatibility TEST
=======================================================

BASE
----
Strictly built from the validated public v0.9.1 / Fresh Core A19 Audit Hardening.

PURPOSE
-------
Restore weapon Manual/AUTO operation in TacSim without restoring the old U74
multi-state architecture.

A19 regression being tested:
- Fresh Core used one strict ResolvePlayer() gate for both weapons and ammo.
- That gate required both a valid ZKntPlayerLoadoutEntity and a runtime playerId.
- GiveWeapon / QueueLoadout do not consume playerId.
- AddAmmo does require playerId.

A19T splits readiness:
- WeaponReady = validated player loadout
- AmmoReady   = WeaponReady + valid runtime playerId

Expected behavior:
- F3/F4/F5-F12 and Overlay Spawn Weapon work whenever WeaponReady is available,
  including TacSim if TacSim still exposes the validated loadout.
- AUTO weapons use WeaponReady.
- F2 and AUTO ammo remain protected by AmmoReady.
- No weapon RID, native spawner, ItemEntry offsets, 500 ms queue delay,
  ammo native path, overlay renderer, or A19 hardening is changed.

TEST
----
1. Confirm normal gameplay Manual + AUTO still work.
2. Enter TacSim.
3. Test F3 and F4.
4. If Auto is enabled, confirm Auto weapons can apply in TacSim.
5. Return to gameplay and confirm no duplicate/broken weapon queue.
6. Send QProtocol.log.

Public release remains v0.9.1 / A19 until this test is validated.
