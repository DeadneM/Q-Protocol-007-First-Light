# Q Protocol v0.9.6 — A25 Accessible Overlay + Native DXGI

This cumulative public build follows **v0.9.5 / A24** and promotes the A25 overlay layout improvement requested and approved by the project maintainer.

## What's new

- **A25 window layout:** increases the default Q Protocol floating window size from `900×650` to `1000×850` ImGui units, bounded by the actual viewport.
- **Pinned bottom buttons:** `Save`, `Reload` and `Defaults` now remain visible outside the scrollable tab region. Users no longer need to discover hidden vertical scrolling to access the main actions.
- **Native DXGI proxy included:** a custom x64 `dxgi.dll` forwards exported functions to the Windows system DXGI and loads the bundled `QProtocol.asi`, reducing reliance on a separate third-party ASI loader.
- **Cumulative features retained:** First Debug/A21 DX12 hardening, A22 reliable mouse input, A23 dynamic ItemEntry/Spawner graph discovery, A24 weapon queue safety, and the existing F1–F12 / AUTO gameplay.

## Preserved behavior

`QProtocol.ini` defaults and the weapon IDs are unchanged from public v0.9.5. No F1 License to Kill, F2 ammunition, F3 manual loadout, F4 Q-Pistol, F5–F12 hotkey, AUTO or native weapon spawn routine has been deliberately changed by A25. Generic reassignment of F1–F12 actions is still not implemented.

## Installation

Exit the game first. Extract the ZIP **directly** into the game binary folder containing `007FirstLight.exe`. It contains exactly these **four root files**:

```
dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt
```

**Back up an existing `QProtocol.ini`** if customized, and **back up any existing game-local `dxgi.dll`**, including ReShade/Special K/other DXGI proxy DLLs. Do not blindly overwrite or stack another DXGI proxy. **Never replace the Windows `System32\dxgi.dll`**. To revert, remove only our game-local DXGI proxy and restore the previous mod DLL/ASI/INI backup.

Our proxy creates `QProtocolDXGI.log`; Q Protocol's gameplay creates `QProtocol.log`.

## Validation

The A25 standalone test and bundled DXGI proxy passed source-isolation, Windows DXGI export and `CreateDXGIFactory1` smoke audits, build/PE import checks, weapon catalogue consistency checks, and ZIP-root checks in GitHub Actions. This public release is based on positive maintainer approval, not a claim that all third-party proxies and PC configurations are certified. Keep the earlier [v0.9.5](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/v0.9.5) available for rollback.

The precise cause of the historical A23 crash has not been proved from the logs; A24 remains a defensive guard. F1 close-combat experiments A20K–A20N are still paused.
