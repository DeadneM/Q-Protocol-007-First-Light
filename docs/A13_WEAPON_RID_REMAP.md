# Fresh Core A13 — October Firearm RID Remap

A13 keeps Fresh Core A5 gameplay and A11 runtime discovery unchanged.

## Current RID remap

| Alias | Previous RID | A13 RID | Current resource |
|---|---|---|---|
| AgencyFocusGun | 01312AEDCB307AA0 | 0198799A3CC4E437 | firearm_instance_waltherppk_agentfocus |
| AssassinHandcannon | 016BEF35840911E8 | 017D8BD5B237B333 | firearm_instance_handcannon_assassin |
| ShotgunStandard | 014B43CF782B9D44 | 01BD00B144B74AC0 | firearm_instance_shotgun_template |
| AssaultRiflePirate | 01D603A2A73ECAAC | 0110285AF7A95A01 | firearm_instance_ar_pirate_a |
| SocomPistol | 01CD24340F11D586 | 01D73DA578C4F423 | firearm_instance_lightpistol_socomsilenced_a |
| LightPistolLargeMag | 012076A6E0D7C1E8 | 010ABE3F66032326 | firearm_instance_lightpistol_largemag_a |
| AssaultRifleNonLethal | 01097D717FAADD1F | 016D12D89A0A658E | firearm_instance_assaultrifle_military_nonlethal |
| SMGNonLethal | 0188220CF173F4C1 | 018C90273080F785 | firearm_instance_smg_military_nonlethal |
| BurstPistol | 012071C1B6B2D734 | 015E2DA84660F7D9 | firearm_instance_burstpistol_3round_a |
| ServicePistol | 01BA9F967A5A6537 | 01988D661ADF3BCE | firearm_instance_servicepistol_a |
| ShotgunCompact | n/a | 01833561121578C4 | firearm_instance_shotgun_compact_a |

Remapped/new entries are Experimental until in-game validation. A13 shows
Experimental entries in Manual/Auto selectors by default.

## Runtime-only cleanup

Brick `0104F2D1C752B7A4` and Vase `01A05C4FEBD7B301` are classified as
Throwable and remain outside WeaponCatalog.

The only A11 runtime RIDs with no Bond-Hashes match are:
- `0142DF24DDF6819F`
- `018DCB210A8B048B`

## Preservation

Previous firearm RIDs are archived in `[WeaponPreviousRid]`.
