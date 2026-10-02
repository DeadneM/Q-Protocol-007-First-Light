Q Protocol - Fresh Core A7
==========================

TEST BUILD

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay base.

A7 changes only the configuration/overlay surface and firearm catalogue metadata.
GiveWeapon, AddAmmo, ResolvePlayer, F1-F12 and AUTO gameplay primitives remain the
same validated core.

True in-game overlay
--------------------
A6/A6B used a native Win32 window above the game. That approach is retired.

A7 uses a DirectX 12 / Dear ImGui overlay rendered directly inside the game's
swap chain. It has no Windows title bar and does not create a visible external
configuration window.

Insert opens/closes Q Protocol.

Tabs:
- Loadout
- Weapons
- Hotkeys

Loadout
-------
Manual:
- F2 reserve ammo
- F3 Q-Pistol / One-handed / Two-handed loadout
- six reserve-ammo values

Automatic:
- enable/disable automatic application
- independent Q-Pistol / One-handed / Two-handed loadout
- independent reserve-ammo values

By default, loadout selectors show only weapons whose Q Protocol spawn path has
been validated. Enable "Show experimental weapons" to expose internal/debug
entries.

Recovered firearm catalogue
----------------------------
The complete known Q Protocol firearm catalogue remains in [WeaponCatalog].

A7 adds:
[WeaponRole]
- QPistol
- OneHanded
- TwoHanded

[WeaponValidation]
- Validated
- Experimental

The Weapons tab displays every known alias, role, validation state and TEMP RID.

Important:
"Experimental" does not mean fake. It means the weapon is a genuine internal
firearm but its source graph has not been proven reliable in every mission.

Current A6B log evidence
------------------------
AUTO itself correctly queues OneHanded.

The failure observed with some selections is the current GiveWeapon source-graph
limitation:
- BurstPistol -> source graph not found in the tested mission
- AgencyFocusGun -> source graph not found
- SocomPistol -> source graph not found

Other tested weapons completed normally, including HeavyPistol50Cal,
MachinePistolHighRecoil, ARMilitary, ShotgunSemiAuto, ARExotic, MarksmanRifle,
SMGFastFire and ShotgunPump.

Test A7
-------
1. Launch a playable mission.
2. Press Insert.
3. Confirm Q Protocol is rendered inside the game with no external window/title bar.
4. Confirm mouse and keyboard input work in the overlay.
5. Open Weapons and verify the full catalogue is visible.
6. Confirm normal loadout combos show validated weapons by default.
7. Enable experimental weapons and confirm the additional entries appear.
8. Save Manual settings and test F2/F3.
9. Save Automatic settings, respawn/change level, and test AUTO.
10. Confirm F1/F4/F5-F12 still work with the overlay closed.
11. Confirm no crash on resize, Alt-Tab, mission transition or shutdown.

If the overlay does not appear or crashes, attach QProtocol.log.
