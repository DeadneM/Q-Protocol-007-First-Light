# Q Protocol v0.9.7

Stable promotion explicitly approved by the maintainer after testing A28/A29.

- Beta updates are **OFF by default**. Startup checking remains ON.
  Existing explicit updater preferences are retained; no personal INI reset.
- In-menu checking, stable/beta selection and confirmed installation after game
  exit, with cancellation, verified downloads and backup/rollback on failure.
- Clearer responsive menu, with accessible Save / Reload / Defaults.
- November-compatible native A28 core, A26 DX12 queue capture and A27 x64 scanner.
- ZIP root contains exactly `dxgi.dll` and `QProtocol.asi`. No INI or README payload.
  Default INI and updater helper are embedded; an existing INI is preserved.

Target EXE SHA-256:
`6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1`
(Steam build 26252352). Compatibility checks remain enabled.

With the game closed, back up the current two mod binaries and extract the ZIP
beside `Retail/007FirstLight.exe`. Keep your INI. Or use A29's Updates > Check now
and confirm Update on exit; close the game normally once staging is complete.
For rollback, close the game and restore the two saved binaries.

Previous v0.9.6, A28 and A29 releases stay intact. The affected-user GPU
DEVICE_LOST incident remains open; no universal GPU fix is claimed. The actual
post-exit update cycle is still subject to runtime confirmation; fixture tests
and public package-discovery/download tests are recorded separately.
