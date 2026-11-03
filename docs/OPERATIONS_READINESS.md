## 2026-11-03 - A31 available through the in-mod BETA channel

User explicitly requires discovery/installation inside the mod with Beta builds
checked. The previous manual-only publication was a misunderstanding and is
superseded: the A31 prerelease now includes QProtocol-update.json, build 31.
Existing ZIP unchanged: SHA-256
0797c3c50215c743aa4cb080d0227b220441d4e83027924457b225db16a37c45.
Publication workflow 41648373006 SUCCESS. The unchanged real updater engine,
using current build 30, finds A31 with beta ON, downloads/verifies its package,
excludes A31 on stable, and does not reoffer it to build 31. No game changes or
launch performed. Main/v0.9.7 preserved. A31 remains diagnostic only.
User path: Updates > Beta builds ON > Check now > Update on exit.

## Latest clarification - 2026-11-03

The maintainer clarifies: player recognition works in the lobby but fails in all
Tac Sim missions they tested. Reproduction does NOT require a preceding driving
mission. Earlier driving/Operations context is retained below as history only.
The READY/spawn lines in the saved session must not be cited as proof of Tac Sim
mission functionality. Campaign/non-Tac-Sim mission behavior is not established
by this clarification.

Compare lobby READY with the NOT READY stage after entering any Tac Sim mission.
The active-player/loadout resolution path is the investigation target, not a
proven root cause. A31 remains observational; no new build or installation is
needed merely to change this reproduction description. No game launch or file
replacement has been authorized/performed for A31.

## 2026-11-03 - Operations readiness failure OPEN; A31 diagnostic isolated

Maintainer reports NOT READY after loading Operations following a driving mission.
Further clarification: it appears specific to the new Tac Sim mission; this is
the target reproduction case, not yet a proven cause or universal restriction.
Saved v0.9.7 session was initially READY, completed F3 and multiple weapon spawns,
then lost readiness and ignored F2-F12. Installed EXE/ASI/DXGI match published
v0.9.7 and its supported executable; personal INI hash remains the baseline.
Session evidence is kept locally in `evidence/player-not-ready-20261103/`,
not committed or uploaded. No game process was present
at inspection, and the existing log does not identify the failed resolver gate.

A31 DIAGNOSTIC on `experiment/a31-operations-readiness` adds bounded reason/handle/
type/runtime-identity logging only. Objective: identify the failing check in that
Operations transition. Native behavior, all guards, UI, updater and INI stay
unchanged; no forced READY or stale-pointer fallback. Scope audit PASS; Windows
build pending. It is NOT a fix and will not be offered by automatic updating
(no public update manifest). No installation or launch performed. Main/v0.9.7
remain preserved. Test requires the same Operations case and a fresh log after
15 seconds; do not repeatedly exercise disabled actions.

### A31 delivery - 2026-11-03

Windows CI 41646734067 SUCCESS on diagnostic source 7cd008ceff6ea3f020588705198105f66e345894.
Downloaded and checked: exact two-file ZIP, CRC, AMD64 binaries, A31 log marker,
585062 bytes, SHA-256 0797c3c50215c743aa4cb080d0227b220441d4e83027924457b225db16a37c45.
Manual-only prerelease: a31-operations-readiness-diag, with no updater manifest.
Status BUILT / CHECKED / OBSERVATIONAL, not a fix and not installed/tested in game.
Next: install after authorization, load the same Tac Sim case for 15 seconds,
read the fresh readiness reason, then fix the evidenced native resolver path.
Raw session evidence stays local; main and stable v0.9.7 remain unchanged.
