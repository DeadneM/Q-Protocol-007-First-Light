Q Protocol - Fresh Core A6B
===========================

TEST BUILD

Validated base
--------------
Fresh Core A5 remains the canonical gameplay base.

Why A6B exists
--------------
A6 proved that the Insert overlay, mouse input and INI persistence work, but
its first layout exposed too much internal configuration and was visually
cluttered.

A6B keeps the same renderer-independent Win32 overlay architecture while
simplifying the UI and the semantics.

Clean UI
--------
The overlay now has two clear panels:

MANUAL
- F2 = ManualAmmo
- F3 = ManualLoadout
- Q-Pistol / One-handed / Two-handed selectors
- six reserve-ammo values

AUTOMATIC
- one Enable checkbox
- AutoLoadout Q-Pistol / One-handed / Two-handed
- six AutoAmmo values

Removed from the visible UI:
- F2/F3 Profile selectors
- AUTO DONE/WAITING debug state
- weapon queue debug state
- Q-Pistol next-mode debug state
- duplicate internal title

Profile= is no longer part of the default control flow.
F2 and F3 use Manual values directly, matching the validated A5 behavior.

Controls
--------
Insert  -> open/close overlay
Save    -> write values to QProtocol.ini and reload runtime config
Reload  -> discard unsaved UI changes and reload QProtocol.ini
Defaults-> restore default Manual/Auto values

While the overlay is visible, F1-F12 manual actions are suppressed.

Preserved from A5
-----------------
- F1 License To Kill
- F2 native ManualAmmo
- F3 typed ManualLoadout
- F4 Q-Pistol swap
- F5-F12 weapons
- shared GiveWeapon queue
- 500 ms inter-weapon stabilization
- AUTO using AutoLoadout + AutoAmmo through the same gameplay primitives

Test
----
1. Press Insert and confirm the compact two-column layout is fully visible.
2. Confirm all buttons are visible without scrolling/cropping.
3. Change Manual values, Save, close with Insert, then test F2/F3.
4. Change Automatic values, Save, then respawn/change level and verify AUTO.
5. Test Reload and Defaults.
6. Confirm F1-F12 do not fire while the overlay is open.
7. Confirm no regression in A5 gameplay.
