# Custom DXGI loader experiment (after v0.9.5)

Status: **independent experimental test**, not a new stable Q Protocol gameplay version.

## Objective

Load the validated **exact v0.9.5 `QProtocol.asi`** without depending on a separate third-party ASI injector/loader. Package a game-local **64-bit `dxgi.dll` proxy** that loads the official Windows System32 DXGI implementation by absolute path and forwards all of its exported ABI.

## Design

- Branch: `dev/dxgi-native-loader-test`. Public stable `main` and existing release `v0.9.5` unaffected.
- `tools/generate_dxgi_proxy.py`: parse the Windows x64 CI machine's **real System32 `dxgi.dll` export table** via `dumpbin /exports`. Generate x64 MASM ABI-preserving thunks and an exact named/ordinal export .DEF, plus a C++ export manifest. This avoids a manually guessed/partial export list.
- `src/dxgi_proxy/ProxyDxgi.cpp`: only saves its own `HMODULE` under `DllMain`. On the **first forwarded DXGI API invocation**, use `InitOnceExecuteOnce` to load the original system DXGI DLL, resolve native exports by name or ordinal, load adjacent `QProtocol.asi` and write `QProtocolDXGI.log`. No `LoadLibrary` from DllMain, no copied Microsoft system DLL, no new game hooks in the loader.
- `src/dxgi_proxy/DxgiSmoke.cpp`: small CI smoke program to call `CreateDXGIFactory1` through the proxy (with ASI loading disabled by a CI-specific environment flag). Original signature, calling convention, RCX/RDX/R8/R9, XMM0-XMM5 and caller stack arguments preserved.
- The game ASI remains exactly the stable v0.9.5 release ASI (unmodified file and hash).
- Standalone ZIP root: `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt`.
- The proxy log is **newly created per run**, not cumulative. The original `QProtocol.log` remains owned by the existing ASI.
- Graceful if ASI is missing: log Windows loader error and continue forwarding system DXGI, without a hard dependency on Q Protocol.

## Open compatibility risks

- **Other DXGI proxies:** cannot coexist as two files at the same path. Back up existing ReShade/Special K/other `dxgi.dll` before testing. Do not overwrite Windows' own DLL.
- **Windows versions:** the named DXGI exports forward dynamically by their names; the ordinal-only exports may not match between different Windows versions. Generated from current Windows x64 CI host and audited, not guaranteed universal.
- **Timing:** the ASI is loaded during the first DXGI export call, outside loader lock. Any ASI hook that depends on observing an earlier event might not yet be installed; check both loader and mod logs after test.
- **Security:** standard Windows DLL search order can be affected by OS policy, anti-cheat, other injectors and protected processes. This prototype is not intended as a bypass.
- **DLL loader lock:** `DllMain` intentionally performs no LoadLibrary, mutex waits or complex initialization.

## Validation checklist

1. GitHub Actions generates/export-audits the loader and builds the smoke test.
2. `QProtocolDXGI.log` says System32 loaded=yes, ASI loaded=yes.
3. `QProtocol.log` reports game EXE accepted, hooks installed, DirectX overlay ready.
4. Verify Insert open/close, click, drag/resize, F1/F2/F3, AUTO, F4/F5-F12; no regression.
5. If no overlay/game crash, collect both logs. **Do not promote custom loader to public stable until tested.**
