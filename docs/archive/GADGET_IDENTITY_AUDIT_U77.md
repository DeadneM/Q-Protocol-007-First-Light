# ARCHIVED

> Retired gadget-era research. Not part of the active Q Protocol architecture.

# Gadget Identity Audit — U74 / U76 / U77

Date: 2026-09-28

## Status

- **U74** is the current functional gameplay base for gadget work.
- **U77** is the current diagnostic build.
- **U75** is not promoted as a gameplay base.
- **U76** was diagnostic only.
- `0x0A0100XX` values are **runtime handles only** and must never be treated as gadget IDs.

## U74 scanner correction

U67 scanned a ±512 MiB window centered on the runtime player object. This worked only while the player allocation happened to remain close to the gadget object region.

U74 changed only the scan window to:

```text
0x150000000 .. 0x1B0000000
```

This restored the gadget census after player relocation while keeping the U67/U57 gameplay paths frozen.

U74 QProtocol.asi SHA-256:

```text
8c442ca40cd89b34588cfe9e43801e03952af8012e3c23e5c3f09ce7d355199a
```

## Why U76 was still too late

U76 captured the four-record producer input and compared controlled manual wheel changes.

The producer record layout observed was:

```text
+0x04 slot
+0x08 FFFFFFFFFFFFFFFF
+0x10 temporary runtime handle
+0x14 state
```

The static audit of the vanilla caller confirmed that `record+0x08` is deliberately written as `-1` by the game. Therefore the producer record cannot provide stable gadget identity.

## U77 upstream hook

U77 moved the diagnostic observation point to the vanilla selected gadget object at:

```text
EXE + 0x16CDC60
```

Immediately before the producer call, vanilla has the selected runtime gadget object available.

The useful object chain is:

```text
selectedObject +0x08 -> child
child +0x48          -> stable Definition RID
selectedObject+0x108 -> stable runtime-object fingerprint
selectedObject+0x120 -> temporary runtime handle only
```

The debugger intentionally labels `+0x120` as a runtime handle and never uses it as an identity.

U77 QProtocol.asi SHA-256:

```text
b16a193fe2cd700b6c7acfe90152fba13b503cec7ca54708b6c978ee4cfddaf1
```

## Controlled manual test

### Set A

```text
Left  = Laser
Up    = Flash Mine
Down  = Smoke Pellets
Right = Shockwave Camera
```

Confirmed:

| Slot | Gadget | object+0x108 fingerprint | child+0x48 Definition RID |
|---|---|---|---|
| Left | Laser | `FB43AC6301B232DF` | `01DAA5F042213007` |
| Up | Flash Mine | `D3DDA43F019DD4A2` | `01996C3564BAC637` |
| Down | Smoke Pellets | `AED6146A0137DAD1` | `01806F52640773E4` |
| Right | Shockwave Camera | `2433248601BCD2B3` | `017B6AC5904D1002` |

### Set B

```text
Left  = Hack
Up    = Dartgun / Flechettes
Down  = Missile Pen
Right = Empty
```

Confirmed:

| Slot | Gadget | object+0x108 fingerprint | child+0x48 Definition RID |
|---|---|---|---|
| Left | Hack | `A04C60F401FBB751` | `01C3E1A95580C73B` |
| Up | Dartgun / Flechettes | `55DDE49601591F97` | `01BA073B3AF0DF1A` |
| Down | Missile Pen | `7535800D013167C9` | `01793C25053E8A06` |

Clearing the Right slot generated no replacement assignment event. Gadget removal therefore follows a separate vanilla branch from gadget assignment.

## Confirmed stable gadget catalogue

```text
Dartgun / Flechettes  = 01BA073B3AF0DF1A
Flash Mine            = 01996C3564BAC637
Laser                 = 01DAA5F042213007
Shockwave Camera      = 017B6AC5904D1002
Smoke Pellets         = 01806F52640773E4
Missile Pen           = 01793C25053E8A06
Hack                  = 01C3E1A95580C73B
```

## INI naming plan

Preferred canonical names:

```ini
Dartgun
FlashMine
Laser
ShockwaveCamera
SmokePellets
MissilePen
Hack
```

Planned backward-compatible aliases:

```text
BlastDevice -> FlashMine
ShockWave   -> ShockwaveCamera
QuickHack   -> Hack
Smoke       -> SmokePellets
Darts       -> Dartgun
```

Current parser support for `FlashMine` and `ShockwaveCamera` is still pending. Until that parser change lands, active slot values should use already-supported names.

## Current Q Protocol target wheel

```ini
Left=MissilePen
Up=Dartgun
Down=SmokePellets
Right=Hack
```

## Next technical target

Do not redesign the scanner again.

The next task is to trace and reproduce the **vanilla slot-assignment path upstream of the producer**, using the confirmed selected gadget object / Definition RID relationship while preserving:

- U74 fixed scanner arena;
- U67/U57 AUTO lifecycle;
- F2 wrapper;
- q31 remapper;
- weapons;
- reserve ammo;
- abilities;
- License To Kill.
