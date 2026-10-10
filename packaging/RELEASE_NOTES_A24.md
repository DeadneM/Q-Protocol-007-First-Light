# Q Protocol v0.9.5 — A24 Overlay Compatibility & Weapon Queue Safety

Public release promoted from **A24**, following a positive in-game report from the project maintainer.

## Changes since v0.9.4

- **First Debug / A21 DX12 renderer improvements:** hardened DirectX 12 overlay initialization, shader/pipeline checks, swapchain and resize handling, safe failure behavior, precompiled shader bytecode and statically linked runtime.
- **A21 floating overlay:** single ImGui window with a title bar, drag/resize and close control.
- **A22 mouse input improvements:** reliably dispatch Win32 click messages to ImGui, avoid dropping mouse clicks under renderer-lock contention, restore gameplay input when closed and retain game/Steam Overlay compatibility precautions.
- **A23 adaptive weapon graph discovery:** search the original graph range plus a bounded region around the current player's live loadout, avoiding failures when game memory allocations move. The A23 log recovered **41 linked weapon graphs**, with **42 item RID entries** and **43 spawners**.
- **A24 weapon request guard:** suppress duplicate active or queued weapon RIDs, cap standalone F4–F12/Debug Spawn pending requests to **4**, and cap accepted standalone requests to **12 per rolling 30 seconds**. This prevents repeated hotkey bursts from queuing excessive native spawns.

## Preserved behavior

- F1 native License To Kill ON/OFF
- F2 manual ammunition
- F3 manual 3-role loadout (Q-Pistol, one-handed, two-handed)
- F4 Q-Pistol variant swap
- F5–F12 configurable standalone weapons
- AUTO loadout and ammunition
- Weapon catalogue / Debug Spawn, runtime discovery, and four tabs
- Existing public `QProtocol.ini` values and weapon RIDs

The game's native spawn routine, regular 500 ms inter-request timing, and F3/AUTO sequencing are retained. F1 close-combat experimental branches A20K–A20N are **not included**.

## Validation and caveats

- All A24 source isolation, configuration, catalog, shader, MSVC build, PE dependency and flat-ZIP packaging audits passed in GitHub Actions **#140**.
- The maintainer reported **“tout a l'air bon”** after testing A24 and requested a stable GitHub release.
- This initial positive report does **not** prove compatibility with every graphics driver, third-party overlay or PC.
- The earlier A23 crash log did **not** capture an exception code or stack. The root cause remains unconfirmed even though A24 mitigates a plausible overload path.
- If you encounter a crash or regression, attach `QProtocol.log` and a Windows crash report rather than assuming an uninstalled third-party DLL is responsible.

## Installation

Extract the ZIP contents into the game's existing Q Protocol installation location, replacing the old mod files. Preserve a backup of any customized `QProtocol.ini` before replacing it.

The ZIP contains **exactly these three files at its root**:

```
QProtocol.asi
QProtocol.ini
README.txt
```

No additional ImGui, MinHook, D3DCompiler or Visual C++ redistributable files are distributed with this build. The earlier **v0.9.4** and A21–A24 test releases remain archived.

More details: [Project state](https://github.com/DeadneM/Q-Protocol-007-First-Light/blob/main/PROJECT_STATE.md), [A24 crash audit](https://github.com/DeadneM/Q-Protocol-007-First-Light/blob/main/docs/A24_CRASH_AUDIT.md).
