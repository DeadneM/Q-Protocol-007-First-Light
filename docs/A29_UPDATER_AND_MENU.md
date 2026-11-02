# A29 updater and menu - 2026-11-02

## Base and user decisions

- A28 source/tag `580e57cb1e240df04e1a3f8cdc0fb0b46067fcc2`.
- Installed from a verified package, with backup and personal INI preserved.
- Maintainer: "ok parfait tout a lair de marcher". Fresh logs are archived in
  `evidence/a28-runtime/`: READY, F1 OFF/ON, 41 linked graphs, complete F3 and F2
  six-class native publication. This accepts A28 on the maintainer configuration.
  Broader hardware validation and the other user's GPU failure remain open.
- New development branch: `dev/a29-updater-menu`; no `main` promotion.
- The maintainer explicitly superseded the older four-file ZIP rule: future ZIPs
  contain the two binaries only; README/default INI/history stay on GitHub.
- Reference inspected read-only: Darksiders Genesis `work-debug/src/UpdateFeature`
  and `OverlayUi.cpp` Updates tab. Same user-facing pattern: startup check, beta,
  explicit installation after exit, retained settings and backups.

## Architecture

`src/Updates.cpp` is separate from gameplay. A pinned ASI worker extracts an
embedded managed helper and starts it hidden. Rendering only reads a locked status
snapshot and enqueues actions. GitHub access, hashing, ZIP parsing, waiting and
installation execute outside render/gameplay threads. No native quit hook,
process termination, System32 change or automatic game relaunch is added.

`updater/UpdateEngine.cs` selects published release assets with a strict
`QProtocol-update.json` schema. Stable ignores prereleases; beta includes them.
Only manifests with a larger integer build and the exact current EXE SHA-256
qualify. Known names are `dxgi.dll` / `QProtocol.asi`; URLs are pinned to this
repository's release/tag, with HTTPS-only allowed GitHub asset redirects.
Archive and individual binary size/SHA-256 are checked, along with AMD64 DLL PE
headers, two-entry root layout and duplicate/traversal rejection.

`Update on exit` rechecks release selection against the build the user confirmed,
probes target-directory write access and stages locally. The helper keeps a handle
to the original parent, supports cancellation while waiting and checks for another
game session before applying. Source hashes are captured before waiting and
rechecked after exit. Backups are verified before replacement; failure after the
first replacement restores changed files. INI and README are never payload files.
No downgrade is performed on a channel switch.

State files and extracted helpers are under `%LOCALAPPDATA%/QProtocolUpdater`.
`status.txt` records checking, available, downloading, staged, cancelled, installed
or error. Backups remain under `Retail/QProtocol-Updates/before-*`; `backup.json`
records previous binary hashes. Restoring two old binaries is manual with the
game closed. No automatic helper elevation is attempted; write denial is reported.

`QProtocolResources.rc` embeds the helper (101) and authoritative source INI (102).
The core uses CREATE_NEW to seed a missing INI after EXE validation and before
loading settings. Existing bytes are preserved. New updater options are read with
defaults and written only when changed: `[Updater] CheckAtStart=1`, `Beta=1`.
Profile Save/Reload/Defaults retain their previous semantics; updater preferences
save immediately, as labelled in the Updates tab.

## Scope and acceptance

`tools/verify_a29_scope.py` compares against A28. Core changes are limited to
bootstrap/update lifecycle and build text. Native map/gates, A27 scanner, A24
request guards, AUTO, F1-F12, assembly, INI, native DXGI and DX12/input stay frozen.
Presentation changes are restricted to DrawLoadoutTab, DrawOverlayWindow,
ApplyStyle and the new Updates tab. DrawFooter remains unchanged.

`tests/UpdaterTests.cs` covers manifest/channel/version/compatibility rejection,
package integrity, x86/traversal rejection, partial-install rollback, temporary
cleanup, old-module refusal and byte-exact INI/README preservation using fixtures.
`tests/MenuSmoke.cpp` constructs the actual ImGui menu at three viewport sizes,
the Updates controls, embedded-default creation and existing-INI reuse. It uses
no game, GPU, window or network. These checks are not runtime gameplay proof.

Success still requires maintainer visual acceptance and one real future update:
check at start, stable/beta switch, confirm, wait while the game remains open,
cancel once, then stage and close normally. Inspect updater status, two binary
hashes, untouched INI, retained backups and next-launch version. Do not manufacture
an artificial newer live release merely to claim this runtime test passed.

## TODO

- Windows CI: helper tests, resource compile, ASI, DXGI smoke, CPU menu smoke,
  dependency audit, two-file ZIP and published manifest verification.
- Verify downloaded package and embedded helper/default INI independently.
- Actual in-game menu, startup check and post-exit update test pending.
- Stable channel becomes available only when a compatible manifest is deliberately
  released as stable. Existing incompatible v0.9.6 remains immutable.
