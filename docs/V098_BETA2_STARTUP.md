# v0.9.8-beta.2 — defer menu GPU initialization

This candidate addresses the loading-stage GPU crash reported with beta.1. The maintainer confirms the game starts without the mod. Two saved Windows dumps stop in the game's GPUCrashReport fatal path at EXE+0x00D00D97, reached from render-frame fence waiting. The mod log ends after an overlay DX12 initialization, resize and reinitialization, before player availability or weapon pickup dispatch. The minidumps lack the heap text containing the exact GPU reason. Attribution to a particular GPU command remains open.

## One behavior change

The overlay used to initialize its GPU resources and submit its font upload before checking whether the menu was visible. Loading-screen resize then led to another initialization while the menu remained closed.

Check the user's visibility request before initializing the renderer. A closed menu during startup does not create its font/PSO/render resources. On the first requested opening, initialization completes before the first visible frame. The input toggle is polled independently by the worker and remains available before renderer initialization.

Scope checks compare the whole source/config/updater/test tree with frozen beta.1 source `96cd470`. Only this six-line early return, version identity and the generic log banner differ in runtime code. The native delivery experiment and compatibility guards are retained. Stable behavior reference remains v0.9.7; the carried character/session and delivery changes still require gameplay acceptance.

## Acceptance sequence

1. Start and enter the Tac Sim lobby with the mod menu closed. The log should have no overlay `Init candidate`, font initialization or render submissions during this closed-menu loading stage.
2. Once Bond is controllable, open the mod menu once and close it. Check that initialization and the first frame complete without GPU loss.
3. Only after those two checks, press F3 once and verify the three actual weapons, including the silenced Q-Pistol. Inspect receipt confirmations and AUTO ammo.

Status **EXPERIMENTAL**. Deferral is a focused change supported by the startup trace. It is not proof of a repaired GPU driver/device, and the first explicit menu opening remains part of acceptance. ZIP contains only `dxgi.dll` and `QProtocol.asi`; personal INI and bindings are retained.

## Verified build and installation

Frozen source `c12ee2de524e7bb2988897f68603205e65d64a3a`, [full CI run](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/42070394975) successful. ZIP 600042 bytes, SHA-256 `7736c6fd0f65578ddf49861b5a2d9a6bd47047d76b873e64d38cf0577d15e85b`. Local archive, x64 PE and per-file hash checks passed.

Installed game/updater closed with a verified backup of beta.1. Installed binary hashes match the checked artifact; personal INI hash is identical before/after. No game launch by the agent. Status **BUILT / CHECKED / INSTALLED / EXPERIMENTAL**. Startup and first-menu acceptance remain pending.

[Beta.2 release](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/v0.9.8-beta.2) verified; publication run `42071005261` successful. The unchanged updater engine discovers/downloads/verifies this ZIP for the current EXE, excludes it on stable and does not reoffer build 40. Beta.1 remains withdrawn in draft.
