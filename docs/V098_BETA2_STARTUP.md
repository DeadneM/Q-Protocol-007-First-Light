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
