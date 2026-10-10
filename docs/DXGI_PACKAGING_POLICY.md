# Q Protocol: native DXGI packaging policy (from A25 onward)

Status: approved user request on 2026-10-10: **“met notre dxgi avec toutes les prochaines builds”**.

## Canonical future-build contract

Every newly generated Q Protocol mod/test/release ZIP must include these **four files at archive root**, not inside a named subdirectory:

```
dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt
```

- `dxgi.dll`: Q Protocol's own x64 native DXGI proxy generated from `src/dxgi_proxy/ProxyDxgi.cpp` + `tools/generate_dxgi_proxy.py`, forwarding the exact system DXGI export names/ordinals; loads adjacent ASI outside DllMain.
- `QProtocol.asi`: the current version's gameplay/overlay build. Do **not** substitute an older ASI merely because it accompanied the first custom DXGI test.
- `QProtocol.ini`: the latest cumulative INI, preserving existing/default user configurations. Do not omit it.
- `README.txt`: per-build changelog and install guidance, including the warning about pre-existing `dxgi.dll` (ReShade/Special K/other loaders) and rollback instructions.

## Build gates

- Generate DXGI forwarding stubs from system DLL exports at compile time.
- Compile x64 `dxgi.dll` using the pinned source in the repository, static C runtime.
- Check exported names AND ordinals against the host system DLL.
- Perform a native `CreateDXGIFactory1` smoke test through the proxy, with ASI loading temporarily disabled **for this CI smoke test only**.
- Verify that `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt` are all present at ZIP root. Missing DXGI or nested paths must **fail** the build, never silently degrade to an ASI-only package.
- Keep the DXGI loader's source code separate from the ASI's gameplay logic, F1-F12, AUTO and HUD.
- Do **not** create/overwrite a numbered public release tag from a generic CI push. The former public `v0.9.5` archive remains as originally released; future numbered releases use a new version tag after in-game approval.

## Source continuity

- Originally developed and verified by GitHub Actions in `dev/dxgi-native-loader-test`, native proxy test `dxgi-native-test`. The build-host DXGI exported 20 functions and `CreateDXGIFactory1` smoke test passed.
- First cumulative A25 archive updated on branch `dev/a25-overlay-fit-default`: DXGI included and audited, with unchanged A24 core gameplay.
- Canonical future build pipeline is `.github/workflows/build-fresh-core.yml` and template `packaging/README_NEXT_DXGI.txt`.

## Installation precautions

Back up a game-local `dxgi.dll` before replacement. Only one DLL at that path can load normally; automatically chaining a ReShade or Special K proxy is not guaranteed. Never overwrite any Windows `System32` files. The new proxy creates `QProtocolDXGI.log`, while gameplay continues to use `QProtocol.log`.

This establishes a default package contract. It does not certify runtime compatibility with all Windows versions, injectors, drivers or game builds.
