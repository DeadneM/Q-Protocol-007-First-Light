# A31 Operations Readiness DIAGNOSTIC

This is an observational TEST based on v0.9.7, **not a readiness fix**.
Reported reproduction: load Operations after playing a driving mission.

The saved v0.9.7 log shows an initially READY player, complete F3 and multiple
weapon spawns, followed by persistent NOT READY. Installed EXE/ASI/DXGI hashes
match the supported executable and published v0.9.7. The old log does not record
which resolver check failed; the game was no longer running during inspection.

Objective: identify the exact failed resolver gate and compare it with the
independent runtime player ID chain, without forcing READY, reusing an old player
pointer, calling new native functions or altering gameplay/renderer/input/INI.
`[A31 READY DIAG]` logs reason, handle/type and runtime root/node/ID, on reason
change and at most every ten seconds while NOT READY. Existing guards remain.

Manual test after installation/launch approval: load the same Operations case,
remain in mission for at least 15 seconds and keep QProtocol.log. No repeated F-key
tests are required while NOT READY. Stop on unexpected behavior. Success means
the failure stage is identified, not that gameplay is repaired.

ZIP contains only dxgi.dll and QProtocol.asi. Back up the two installed v0.9.7
binaries with the game closed; preserve the existing INI. Restore that pair to
roll back. README stays on GitHub. No QProtocol-update.json is published, so this
diagnostic is never automatically offered to stable or beta updater users.
Stable main/v0.9.7 stay unchanged. Build/runtime states are recorded separately.
