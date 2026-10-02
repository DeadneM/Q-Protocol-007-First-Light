Q Protocol - Fresh Core A3D
===========================

TEST BUILD

Validated base
--------------
Fresh Core A2 remains canonical.

A3C result
----------
QProtocol.log proves all three typed F3 requests were queued and each native spawner cycle returned to idle, but only the Q-Pistol was actually retained by the game.

This means our previous "GiveWeapon COMPLETE" criterion was too early for sequential loadout construction: spawner idle does not guarantee the gameplay/loadout layer has finished integrating the weapon.

Historical evidence
-------------------
The validated pre-update architecture used:

ManualGiveWeaponDelayMs=500
AutoGiveWeaponDelayMs=500

A3D correction
--------------
After each GiveWeapon completion, the shared weapon queue now waits 500 ms before starting the next request.

No other weapon logic changes.

Typed F3 order remains:
1. QPistol
2. OneHanded
3. TwoHanded

F5-F12 remain independent weapon hotkeys.

Still inactive
--------------
- F2 ammo
- AUTO
- overlay

Test
----
1. Reach a playable mission.
2. Press F3 once.
3. Wait about 2 seconds.
4. Confirm:
   - configured Q-Pistol variant
   - configured one-handed firearm
   - configured two-handed firearm
5. Confirm F1/F4/F5-F12 still work.

Expected log additions:
GiveWeapon queue cooldown = 500 ms
