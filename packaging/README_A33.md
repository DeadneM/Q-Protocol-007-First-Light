# A33 Tac Sim Identity Contract — BETA TEST

Production base: accepted v0.9.7. A32's fallback was not a successful Tac Sim fix.

Goal: recover the typed native loadout in Tac Sim with the correct lookup ID,
then verify actual manual weapon delivery. This is not full ASI hardening.

Read-only lobby/Tac Sim comparison confirmed two distinct IDs in the runtime node.
The game's two native callers pass node+0x34 to helper 0x015805D0. A32 passed
node+0x30, which matches another field of the player spawner. In Tac Sim both
context interfaces lack their nested loadout, while the native player registry
still exposes a correctly typed loadout via its matching +0x1AC lookup ID.

A33 passes the spawner ID at +0x34 only to the mission lookup. It still returns
the existing +0x30 runtime ID to shared weapon/ammo code. Both IDs and root/node
must remain stable during the call; the native invalid-ID sentinel is rejected.
The original successful resolver path, full 701-byte helper code guard, PE/code/
type compatibility checks and rejection of null/wrong-type results are retained.
No remembered lobby pointer or forced READY is introduced.

Preserved: F1–F12, weapon catalogue/RIDs, spawn timing, AUTO sequencing, ammo
publication, UI/input/DX12, updater behavior and personal INI. Beta remains OFF
by default. No new files are written into the personal configuration by the ZIP.

Install through the existing mod menu: Beta builds ON > Check now > Update on
exit. ZIP contains exactly dxgi.dll and QProtocol.asi at its root. README and
defaults remain on GitHub. Close normally to apply the staged update; restart
the game yourself. No hot replacement of loaded binaries is supported.

Test: first check lobby, then load Tac Sim and verify READY. Press F3 once and
verify the actual requested weapons are received. Stop on a crash, missing
weapons or unexpected repeated grants; retain QProtocol.log privately.
Acceptance requires actual delivery in Tac Sim and no lobby regression.

Known open work: the existing AUTO latch may already have fired in the lobby
and does not guarantee another run on entry into Tac Sim. COMPLETE means native
spawner processing ended, not verified inventory delivery. Session/request
correlation, mailbox resets and AUTO acknowledgement remain follow-up audit
items; A33 does not claim to resolve them. Test F3 before drawing AUTO conclusions.

Status: EXPERIMENTAL, user runtime validation pending. Main/v0.9.7 unchanged.
Rollback with the game closed: restore the previous pair from the verified
Retail/QProtocol-Updates/before-* backup. Keep QProtocol.ini. The monotonic
updater does not automatically downgrade build 33 to stable build 30.
