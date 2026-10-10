Q Protocol v0.9.5 - Custom DXGI Loader TEST (x64)
====================================================

PURPOSE
-------
This standalone DXGI proxy loads QProtocol.asi without a third-party ASI
loader. It dynamically forwards native DXGI exports to the authentic Windows
System32 dxgi.dll. The Q Protocol gameplay and overlay binary is EXACTLY the
same as the public v0.9.5 release.

FILES (directly at the root of this ZIP)
----------------------------------------
dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt (this file)

INSTALLATION
------------
1. EXIT 007 First Light.
2. Back up your existing QProtocol.ini if customized.
3. Locate the folder containing the main game executable 007FirstLight.exe.
4. If there is already a dxgi.dll in this folder, BACK IT UP instead of
   overwriting it. Do not install two competing DXGI proxy mods together.
5. Extract the four ZIP files next to 007FirstLight.exe.
6. Launch the game normally. The first DXGI API call will trigger this
   proxy's initialization and it will load QProtocol.asi from the same folder.

OBSERVABILITY
-------------
A separate QProtocolDXGI.log is generated in the same folder, with details
of System32 DXGI loading and whether the ASI was loaded or already present.
QProtocol.log continues to be written by the ASI as in v0.9.5.

SAFETY / COMPATIBILITY
----------------------
- Only for 64-bit Windows/007 First Light.
- Does not change the Windows/System32 dxgi.dll or any game executable.
- Real DXGI is loaded by absolute System32 path, and exported API calls
  transparently forward to corresponding native functions.
- DXGI ABI/export list generated from the build host's Windows system DLL
  and audited by CI, but cross-version and other injector compatibility
  remain unproven. Do not overwrite ReShade, Special K or any other dxgi.dll.
- This custom loader is NOT required to use ordinary Q Protocol v0.9.5.
- To revert: remove only this supplied local dxgi.dll and restore any prior
  third-party proxy you backed up. The old ASI loader, if present, can then
  be restored. Avoid removing Windows system DLLs.
- QProtocol.ini is unchanged from v0.9.5 (SHA-256
  8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022).
- The DXGI proxy has no ImGui menu of its own. The menu is still Q Protocol's
  DX12 overlay opened by Insert or the configured toggle key.

STATUS
------
Experimental. Dynamic export forwarding has been compiled/audited and must
be verified with the real game. Main v0.9.5 public release is untouched.
