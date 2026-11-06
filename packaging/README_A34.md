# A34 Playable Player Core — BETA TEST

Rebuilt from accepted v0.9.7. A32/A33 loadout fallbacks are not carried forward.

One shared native-character availability state now gates AUTO and manual F2–F12.
The game-thread observer reads the local character's current typed binding,
generation and player owner, independent of loadout availability. Native
not-alive/sequence membership closes the gate. Snapshots must remain fresh.

AUTO runs once per character incarnation, then waits for the correlated ammo
result. Requests and replies carry session/epoch/request IDs. Scene changes
retire old work; reset cannot race the ammo payload copy or overwrite an
unacknowledged result. Partly interrupted AUTO is failed rather than blindly
repeated. Manual primitives and AUTO still share the same native implementation.

Preserved: F1, hotkeys and bindings, weapon catalogue/RIDs, native spawn timing
and burst limits, ammo classes/amounts, overlay/input/DX12, updater defaults and
personal INI. Additional native code guards retain fail-closed compatibility.

Install through Beta builds ON > Check now > Update on exit; restart yourself.
ZIP contains only dxgi.dll and QProtocol.asi. README/default INI stay on GitHub.
No game launch or installation by the agent. Stable main/v0.9.7 stays unchanged.

Goal/test: in a controllable story mission verify availability, actual AUTO
weapons/ammo and manual F2/F3. Then lobby -> Tac Sim mission: verify the new
session, AUTO and manual again. Check death/reload and return to lobby without
duplicate grants. Stop on unexpected behavior and retain logs privately.

Status EXPERIMENTAL until these user tests pass. Native read contracts and
lifecycle fixtures are not gameplay proof. Spawn idle/processing acknowledgement
does not independently prove inventory delivery. Some mission-specific input
blocks may need further validation; the mod does not force game control flags.

Rollback with the game closed: restore the accepted v0.9.7 binary pair or the
appropriate verified updater backup. Preserve QProtocol.ini. The updater cannot
automatically downgrade a higher beta build to stable build 30.
