# A32 Tac Sim Native Resolver - BETA TEST

Functional candidate based on stable v0.9.7. A31 runtime evidence identified the
failed stage: native handle empty in Tac Sim while runtime player ID stays readable
and nonzero. The lobby path had returned a correctly typed loadout earlier.

A32 preserves the old resolver whenever it succeeds. Only when its native handle
is empty does A32 call the game's existing typed-loadout getter with the current
player ID. This helper supports active context, legacy loader and player-ID lookup.
Its complete 701-byte function is checked before hooks; returned loadout type and
runtime root/node/ID stability are mandatory. No remembered lobby pointer, forced
READY, memory scan, new gameplay hook or compatibility bypass is used.

Beta builds ON > Check now > Update on exit in the mod menu. The prerelease ships
QProtocol-update.json for this purpose; the stable channel does not offer A32.
ZIP contains only dxgi.dll and QProtocol.asi. Preserve the personal INI. Existing
updater backups remain under Retail/QProtocol-Updates; restore old binary pair
with the game closed to roll back manually.

Test once: verify lobby still works, enter Tac Sim, look for PLAYER READY and
`[A32 mission resolver] current typed loadout accepted` in QProtocol.log. Then
test one F2/F3 action and compare actual in-game results. Stop on an unexpected
result. Runtime acceptance is pending; build/fixtures/static mapping are not
proof that Tac Sim gameplay is repaired. Main/v0.9.7 remain unchanged.

After this correction is accepted, the maintainer requests a separate ASI audit
using accumulated GitHub/conversation history. Do not mix that cleanup into A32.
