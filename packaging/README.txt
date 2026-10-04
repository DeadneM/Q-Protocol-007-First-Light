Q Protocol - Fresh Core A19
===========================

FULL AUDIT HARDENING TEST

A19 is a consolidation build. It does not change the validated A5 gameplay
primitives, weapon RIDs, ammo classes or 500 ms weapon stabilization.

Audit fixes
-----------
1. Remappable overlay key is now safe even when mapped to F1-F12.
2. F1-F12 edge state is updated while the overlay is visible.
3. ToggleKey reload latches the real physical key state.
4. Core Auto.Enabled fallback is now 0, matching shipped defaults.
5. Failed DX12/MinHook bootstrap cleans partial hooks immediately.
6. Startup logs no longer falsely claim Insert is hardcoded.
7. CI validates RID format, RuntimeCategory coverage, Auto.Enabled,
   ammo keys/ranges, previous RID integrity and Overlay.ToggleKey.
8. Docs now state the truth: generic F1-F12 Action remapping is planned but
   NOT implemented. Current F1-F4 actions are fixed; F5-F12 weapon aliases are
   INI-backed.

Preserved
---------
- Overlay toggle remapping through [Overlay] ToggleKey.
- QPistol / OneHanded / TwoHanded Off support.
- Automatic defaults: QPistolSilenced + Off + Off.
- Experimental weapons hidden by default.
- Latest user-validated weapon statuses.
- 31 unique weapon RIDs and 10 separate runtime-only RIDs.

A19 is a TEST candidate until in-game validation.
