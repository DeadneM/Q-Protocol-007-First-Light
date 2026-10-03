# A5 vs A7B gameplay audit

Date: 2026-10-03

## Result

The validated A5 gameplay core did **not** drift in A7B.

Git comparison base:
- A5 validation commit: `0617fde91f8b37fa2fd6bc19fc1aeefed4da4fcb`
- compared against the A7B line before A8

## GameplayHook.asm

Exactly identical.

Git blob SHA at both revisions:
`bc1f82c1f37b6d0ea92161a1f2ee2ff201d78295`

Therefore the gameplay-thread hook assembly and its saved/restored registers are
unchanged.

## QProtocol.cpp

The GitHub compare patch changes only:

1. adding `#include "Overlay.h"`;
2. version/scope log strings;
3. overlay initialization/status code inside `WorkerThread`;
4. `OverlayPump()` and optional INI reload handling;
5. wrapping F1-F12 dispatch with `!OverlayIsVisible()`;
6. `OverlayShutdown()` before worker exit.

The compare contains **no changes** in the implementation of:

- executable validation and October RVAs;
- `ResolveNativeLoadout()`;
- `ResolvePlayer()`;
- License To Kill bytes/toggle;
- RID parsing/catalog resolution;
- `LoadConfig()` gameplay parsing;
- graph indexing;
- donor pair cloning;
- native Spawn;
- weapon queue;
- 500 ms inter-request delay;
- AddFirearmAmmunitionToPlayer;
- ManualLoadout;
- AutoLoadout;
- AUTO READY-cycle logic;
- `QpGameplayTick()`;
- DLL attach/detach behavior other than overlay shutdown being called by WorkerThread.

The only runtime gameplay-facing change is intentional overlay input gating:
while a renderer-ready overlay is actually visible, F1-F12 are not dispatched.

A7B changed `OverlayIsVisible()` to fail-open, so if its renderer was not ready,
this gate evaluated false and A5 hotkeys remained active.

## QProtocol.ini

The INI is not byte-identical.

A7/A7B:
- removed the unused/decorative `Profile=Manual` lines under F2/F3;
- added `[WeaponRole]`;
- added `[WeaponValidation]`.

The actual default weapon RIDs, ManualLoadout, AutoLoadout, ManualAmmo,
AutoAmmo, F4 modes and F5-F12 selections remain the same.

## Conclusion

A5 remains the canonical gameplay implementation.

Future overlay experiments must continue to treat renderer code as an isolated
front-end. They must not modify GiveWeapon, AddAmmo, AUTO or the gameplay hook
while overlay rendering is being debugged.
