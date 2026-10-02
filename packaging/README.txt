Q Protocol - Fresh Core A2
==========================

TEST BUILD

Validated from A1
-----------------
- clean ASI bootstrap
- October executable validation
- F1 License To Kill ON/OFF

New in A2
---------
A2 adds ONE shared weapon primitive.

F4:
- swaps between QPistolSilenced and QPistolUnsilenced
- both variants go through the same GiveWeapon() path

F5-F12:
- give the weapon configured in QProtocol.ini
- all eight slots go through the same GiveWeapon() path

The old dedicated Q-Pistol state machine is NOT present.

Still intentionally inactive
----------------------------
- F2 ammo
- F3 manual loadout
- AUTO
- overlay

Architecture under test
-----------------------
Worker thread:
- ResolvePlayer()
- build/cache native ItemEntry/Spawner graph index only when an arm is requested
- temporarily clone requested ItemEntry descriptor into the validated donor
- queue one gameplay-thread spawn

Gameplay hook:
- performs only the native Spawn(spawner) call
- no scanning
- no logging
- no heavy state machine

After the native spawner returns idle, the donor descriptor is restored.

Test
----
1. Reach a playable mission.
2. Verify F1 still toggles LTK.
3. Press F4 several times: Q-Pistol should alternate silenced/unsilenced.
4. Test F5 through F12.
5. Send QProtocol.log.

Expected useful log lines:
- PLAYER READY ...
- Gameplay hook installed ...
- Weapon graph index built ...
- F# queued RID=...
- GiveWeapon prepared RID=...
- GiveWeapon COMPLETE RID=...
- GiveWeapon donor restored (complete): RID=OK template=OK

If the game crashes before using any weapon, the isolated suspect is the new tiny gameplay hook.
If it stays stable but a weapon fails, the log should identify graph resolution / spawner / descriptor stage.
