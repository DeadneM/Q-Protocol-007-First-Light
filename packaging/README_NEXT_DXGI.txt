Q Protocol - Future Cumulative Build with Custom DXGI
===================================================

BUNDLED BY DEFAULT
------------------
All new Q Protocol test and release archives must include four files
directly at the root of the ZIP:

dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt

dxgi.dll is our custom Q Protocol x64 DXGI proxy. It forwards the Windows
DXGI exports to the genuine Windows System32 dxgi.dll and loads the local
QProtocol.asi, without a separate third-party ASI loader.
QProtocolDXGI.log reports native DXGI and ASI loader status.
QProtocol.log reports Q Protocol's game/overlay events.

INSTALLATION / WARNING
----------------------
Exit the game before modifying the installation directory.
Back up any customized QProtocol.ini and any existing local dxgi.dll.
Copy all four files next to 007FirstLight.exe, in the game's working
binary directory. Do NOT replace or delete Windows/System32/dxgi.dll.

Do not overwrite another mod's dxgi.dll (such as ReShade or Special K)
without arranging compatible chaining. Windows only loads one DLL from
a given path/name; these proxies are not automatically compatible.
If an alternative ASI loader already loads QProtocol.asi, disable
duplicate injection rather than stacking loaders blindly.

ROLLBACK
--------
Remove our game-local dxgi.dll and restore the previous local proxy
from backup. For an ASI-only setup, simply omit the dxgi.dll and
use the previous ASI loader if needed. The v0.9.5 stable release
and legacy testing assets are unchanged.

PROJECT FEATURES
----------------
Existing F1-F12, manual loadout, AUTO, ammunition, Q-Pistol swap,
native weapon graph discovery, and DX12 overlay remain part of the ASI.
This packaging change does not modify those features.

CI guarantees the proxy is regenerated using current Windows System32
DXGI's export table, validates all export names/ordinals, runs a
CreateDXGIFactory1 smoke test, and verifies ZIP root file layout.
Actual compatibility across game builds, Windows versions, third-party
overlays and graphics drivers still requires in-game testing.

This README is the template for future cumulative builds; individual
versions must add their gameplay/UI changelog and version number.
