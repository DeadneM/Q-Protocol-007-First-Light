# Q Protocol v0.9.8

Stable promotion of the maintainer-accepted v0.9.8-beta.4 for **007 First Light Steam build 26430236** (game 1.3.2.0).

- Instant native weapon pickup retained for manual commands, including the silenced Q-Pistol.
- First AUTO arrival repaired: owned AUTO creations use native placement completion instead of the source's first-index world-throw operation. Exact ownership, player/session, item identity and compatibility checks remain active.
- F4 alternation advances on confirmed inventory receipt. Existing F1–F12 actions, profiles, safety limits and personal bindings preserved.
- Startup/menu GPU initialization deferred until the menu is requested, preserving the beta.2 startup correction.
- Updater defaults: startup check ON, beta channel OFF. Explicit personal choices are kept. Stable v0.9.8 is offered for the supported executable through both channels.

The maintainer validates beta.4 and explicitly authorizes main/stable promotion. Its final log contains four AUTO packages with all 12 inventory receipts confirmed, including a return after a positive native death observation, with no receipt timeout. Earlier beta.3 trials validated instant manual delivery and included story mode. This release changes only version/build identity in the accepted runtime; the stable package is rebuilt from main and checked by CI.

**Install ZIP: only `dxgi.dll` and `QProtocol.asi` at root.** Close the game before replacing these two files and keep your existing `QProtocol.ini`. Missing configuration is created automatically; documentation and source configuration stay on GitHub. Previous stable/beta tags and assets remain available.

Exact supported EXE SHA-256: `f92c0e8600aa3b692a437f2c0795068f509db1a73a4794b619e6d60bd5d35c43`. Native code/type guards are retained. The full pipeline checks native read/session contracts, owned completion, the actual x64 routing hook, updater transactions, menu/bootstrap, DXGI forwarding, binary dependencies and ZIP integrity. CI is distinct from the maintainer's gameplay validation; unobserved same-identity reloads and the complete per-weapon/per-mode matrix remain in the cumulative technical TODO.
