Q Protocol A26 - Authoritative DXGI Queue Capture TEST
=====================================================

STATUS
------
EXPERIMENTAL based on stable v0.9.6. Do not replace the public release
for normal users. A26 targets the first-frame GPU DEVICE_LOST incident.

FIX UNDER TEST
--------------
DXGI factory creation hooks now identify the command queue actually
associated with the game's swapchain. Only that queue is used to submit
Q Protocol overlay commands. An unknown queue disables overlay drawing
for safety rather than selecting another queue by guess.

DEVICE_LOST now reports the actual driver reason (where available),
rather than an invented generic timeout.

UNCHANGED
---------
Weapon controls F1-F12, manual/automatic loadouts, all native weapons,
the full weapon catalog and QProtocol.ini; A25 larger menu and pinned
Save/Reload/Defaults; A22 mouse capture and First Debug renderer code.

INSTALLATION
------------
Back up your QProtocol.ini and any existing game-local dxgi.dll first.
Extract all four files directly next to 007FirstLight.exe:
C:\Program Files (x86)\Steam\steamapps\common\007 First Light\Retail\

  dxgi.dll
  QProtocol.asi
  QProtocol.ini
  README.txt

Our native x64 dxgi.dll is included. Do NOT overwrite other DXGI
wrappers (ReShade, Special K) without backup and compatibility planning.
Never replace Windows System32 dxgi.dll.

LOGS
----
For problems, send QProtocol.log; QProtocolDXGI.log additionally shows
loader startup. Note whether A26 verified the swapchain queue or skipped
it. Do not confuse HRESULT 0x887A002B device loss with 0x800705B4
timeout, which was a synthetic error in the old overlay code.

ROLLBACK
--------
Restore the official v0.9.6 release or backed-up DLLs and INI.
