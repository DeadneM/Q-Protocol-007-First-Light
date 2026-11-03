## 2026-11-03 - Operations readiness failure OPEN; A31 diagnostic isolated

Maintainer reports NOT READY after loading Operations following a driving mission.
Saved v0.9.7 session was initially READY, completed F3 and multiple weapon spawns,
then lost readiness and ignored F2-F12. Installed EXE/ASI/DXGI match published
v0.9.7 and its supported executable; personal INI hash remains the baseline.
Evidence is in `evidence/player-not-ready-20261103/`. No game process was present
at inspection, and the existing log does not identify the failed resolver gate.

A31 DIAGNOSTIC on `experiment/a31-operations-readiness` adds bounded reason/handle/
type/runtime-identity logging only. Objective: identify the failing check in that
Operations transition. Native behavior, all guards, UI, updater and INI stay
unchanged; no forced READY or stale-pointer fallback. Scope audit PASS; Windows
build pending. It is NOT a fix and will not be offered by automatic updating
(no public update manifest). No installation or launch performed. Main/v0.9.7
remain preserved. Test requires the same Operations case and a fresh log after
15 seconds; do not repeatedly exercise disabled actions.

