# Q Protocol v0.9.4 — Fresh Core A20J Clean Core

Cleanup release built on the fully validated A20I gameplay core.

## Changes

- Removed obsolete donor/pair-clone restoration code and dead weapon state.
- Removed unused License To Kill parser state and overlay telemetry.
- Simplified internal Weapons / Debug overlay plumbing.
- Reused the shared Spawn Weapon queue helper instead of duplicate code.
- Removed Q Protocol's own `WriteProcessMemory` import.
- Preserved the validated direct native `ItemEntry/Spawner` weapon architecture.

## Validation

A20J was validated in game with F1, F2, F3, F4, multiple F5-F12 weapon slots, AUTO, player READY-cycle transitions, the four overlay tabs and repeated Debug Spawn Weapon calls.

No weapon RID, gameplay timing, retry policy, AUTO sequence or public default was intentionally changed.

## Package

The release archive contains directly at its root:

- `QProtocol.asi`
- `QProtocol.ini`
- `README.txt`
