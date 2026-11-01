# 007 First Light — November 2026 executable compatibility port

Status: **BLOCKED on new executable**. Development branch `dev/nov2026-game-update-remap` created after integrating A26+A27 to `main`.
Date: 2026-11-01. The existing stable v0.9.6 package is unchanged and **not certified** for the new game update.

## Current known October 2026 identity

- `007FirstLight.exe` previously accepted with PE `TimeDateStamp = 0x6AC4D653`, `SizeOfImage = 0x06D31000`.
- Current `src/QProtocol.cpp` rejects other headers before installing gameplay hooks. This guard must remain until the game-specific RVA map is verified.
- The initial module load string is historical (`v0.9.4 / A20J`) and must not be mistaken for the current source build tag.
- Previous hardware result: maintainer A27 41 linked graph entries, 42 ItemEntry RIDs, 43 spawners, complete F3 package and F2 ammo. A different user's GPU device still returned `0x887A002B` after A26. Treat these as separate problems.

## Needed before changing addresses

- **The newly patched `Retail/007FirstLight.exe`**, preferably intact.
- Optional `QProtocol.log` from launch with the existing mod; it can show the new header and demonstrate safe refusal, but it is not a substitute for the executable.
- Windows Steam build/channel metadata if available; don't assume two game EXEs are identical based only on PE timestamp and size.

## Order of work

1. Obtain new binary and its SHA-256, PE timestamp/size, section layout; determine whether the old executable is available for binary comparison.
2. Locate new native RVAs using cross-reference/code evidence, *not* a global offset shift. Audit old expected hook preimages against new bytes.
3. Remap the player resolver and loadout vtable; verify READY and player identity, in diagnostic mode if necessary.
4. Remap F1 native License To Kill observe/toggle, maintain safe preimage checks and return semantics.
5. Remap A23/A27 ItemEntry and Spawner vtables, native GiveWeapon/spawn paths and F3/F4/F5–F12. Keep A24 request safety.
6. Revalidate F2 native ammunition classes and AUTO loadout/ammo paths separately.
7. Keep A25 overlay, A26 swapchain queue capture and A27 bounded 64-bit graph scanner; GPU `DEVICE_LOST` remains open and should not be hidden by a timeout.
8. Build Windows x64 MSVC A28 **TEST only** with custom native `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt` directly at archive root; run DXGI export/ABI smoke and frozen-behavior checks.
9. Require in-game test logs before any new numbered stable release. Keep older tags and verified rollback builds intact.

## Constraints

- Never bypass PE gating or install native hooks against a guess.
- Never edit Windows System32 DXGI or overwrite a stable GitHub release.
- Do not add or revive rejected gameplay experiments.
- `PROJECT_STATE.md` on `main` is the canonical history of established decisions.
