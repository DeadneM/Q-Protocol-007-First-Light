# Fresh Core A17 — Optional Loadout Slots

A17 keeps the validated Fresh Core A5 gameplay primitives and the audited A15
weapon catalogue architecture.

## Optional slots

All three typed loadout roles can now be set to `None (Off)` from the overlay:

- QPistol
- OneHanded
- TwoHanded

The existing core already parses `None` as RID 0, so this does not add a new
spawn path or state machine.

## Public Automatic defaults

```ini
[Auto]
Enabled=0

[AutoLoadout]
QPistol=QPistolSilenced
OneHanded=None
TwoHanded=None
```

Reset Defaults and missing-key overlay fallbacks use the same Automatic defaults.

## User validation updates

Promoted from Experimental to Validated from the supplied October INI:

- AssaultRiflePirate
- SocomPistol
- LightPistolLargeMag
- ShotgunCompact
- ShotgunCompactOneHanded
- AssaultRifleNonLethal
- SMGNonLethal
- ServicePistol

All other weapon statuses remain unchanged.
