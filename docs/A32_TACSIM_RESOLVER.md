## 2026-11-03 - A32 Tac Sim native resolver correction TEST

A31 runtime diagnosis identifies native-handle-empty while the independent runtime
player ID remains valid in the reported Tac Sim case. Session logs are kept only
under the local workspace `local-evidence/a31-tacsim`, not committed/uploaded.

A32 starts from stable v0.9.7. Preserve the original path on success; on the exact
empty-handle condition, call existing native helper RVA 015805D0 with the live
uint32 player ID and a 16-byte output reference. Static evidence: both typed
lookups use loadout registry 06792438, output pointer/key at +0/+8, AL success;
callers at 01581668 and 01581EC5 pass the ID in ECX, output in RDX and test AL.
The helper covers two native context routes and the native player-ID lookup
016BAEE0, whose object supplies a typed loadout reference at +188. The old
171D2E0 resolver covers only one of those routes.

The full 701-byte helper preimage is checked before hooks. Require correct loadout
vtable plus stable root/node/nonzero ID before and after resolution, and recheck
the common READY result against that ID. No stale lobby loadout is returned and
no guard is bypassed. UI, renderer, gameplay, updater and personal INI stay frozen.
A32 is a functional candidate, not runtime-validated. It will be published with
its beta updater manifest so the maintainer can install from inside the mod.

Scope audit PASS; native getter guard fixtures and Windows build pending. No
installation/launch. Main and v0.9.7 remain unchanged. Next: one lobby-to-Tac-Sim
acceptance test and actual F2/F3 outcomes. After this correction is accepted,
perform the separately requested ASI audit using accumulated GitHub/conversation
history; do not combine speculative cleanup with this native correction.
