Q Protocol - First Debug Overlay Compatibility TEST
===================================================

STATUS
------
TEST candidate only.

This build targets the compatibility incident where some users report that
007 First Light crashes when opening the Q Protocol overlay.

The crash is NOT declared fixed yet. The public stable release remains v0.9.4.

BASE
----
Built directly from validated Q Protocol v0.9.4 / Fresh Core A20J.

First Debug does not include the experimental A20K-A20N gameplay branches.

GAMEPLAY / CONFIG
-----------------
Unchanged from v0.9.4:
- F1 License To Kill
- F2 ammo
- F3 Manual Loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- weapon RIDs, timing and retries
- QProtocol.ini format and defaults
- Loadout / Weapons / Debug / Hotkeys UI features

The build pipeline rejects First Debug if the validated gameplay source or
QProtocol.ini differs from v0.9.4.

FIRST DEBUG CHANGES
-------------------
The DX12 / Dear ImGui renderer has been hardened without removing the overlay.

- ImGui shaders are compiled at build time and embedded into QProtocol.asi.
- Runtime D3DCompile / D3DCompiler_47.dll dependency is removed.
- Shader, root-signature, pipeline and font resources are created and verified
  before the overlay is considered ready.
- DX12 resource creation failures are checked and logged with HRESULTs.
- Present only renders on the tracked game swapchain.
- ResizeBuffers only rebuilds the tracked game swapchain.
- Present / resize / shutdown renderer lifetime is synchronized.
- Per-frame command allocator reuse is protected by fences.
- GPU resources are not released if safe completion cannot be proven.
- Device-removal reasons are logged when available.
- Unexpected ImGui initialization/render faults are contained at the overlay
  boundary where possible.
- Renderer failure is fail-open: gameplay and gameplay hotkeys remain active.
- Common third-party overlay modules are logged for compatibility diagnosis.
- WndProc restoration avoids overwriting a later hook installed by another
  component.

FIRST-OPEN DIAGNOSTICS
----------------------
QProtocol.log now reports renderer stages such as:
- First Debug renderer bootstrap
- queue / swapchain selection
- device-object initialization
- FirstOpen requested
- FirstOpen render begin
- FirstOpen frame submitted successfully
- DX12 / ImGui HRESULT failures
- Resize and GPU-fence failures

DEPENDENCIES
------------
Dear ImGui v1.91.5 and MinHook v1.3.4 are compiled into QProtocol.asi.

The TEST ASI is built with the static MSVC runtime and its PE dependency audit
requires only Windows / DirectX system components:

- USER32.dll
- KERNEL32.dll
- d3d12.dll
- dxgi.dll
- SHELL32.dll
- IMM32.dll

No separate ImGui, MinHook, D3DCompiler or Visual C++ runtime installation is
required by this build.

PACKAGE
-------
The ZIP contains exactly these files at its root:

QProtocol.asi
QProtocol.ini
README.txt

VALIDATION LIMIT
----------------
Compilation, dependency checks, gameplay-source invariants and package structure
are automated. They do not reproduce every affected user's GPU, driver or
third-party overlay environment.

First Debug remains a TEST candidate until runtime validation is sufficient.
