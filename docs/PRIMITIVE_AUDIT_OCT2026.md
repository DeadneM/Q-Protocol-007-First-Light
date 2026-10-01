# Q Protocol — Primitive Audit for October 2026

Date: 2026-10-01

## Purpose

This document freezes the native primitives required by the new minimal Q Protocol core.

The old U74 cumulative state machines are **not** being ported.

Target executable:

```text
007FirstLight.exe
size    65,068,936 bytes
SHA-256 9d82479246d2c2fbcbcbb94e24aeb91bc8917241148278a3288031afb5828bf2
```

Historical behavioral reference:

```text
U74 QProtocol.asi
SHA-256 8c442ca40cd89b34588cfe9e43801e03952af8012e3c23e5c3f09ce7d355199a
```

## 1. ResolvePlayer()

### Required output

The fresh core should return one small shared context:

```cpp
struct PlayerContext {
    void* loadout;
    uint32_t playerId;
};
```

All F2-F12 player-dependent actions and AUTO use this exact resolver.

### New executable mappings

Validated current targets:

```text
player resolver       EXE+0x171DF40
player registry helper EXE+0x07D7770
player registry global EXE+0x069225A0
runtime player global  EXE+0x064576F8
```

Expected player/loadout type:

```text
ZKntPlayerLoadoutEntity primary vtable = 0x142EDEB70
```

The playerId chain used by the previous validated runtime path is:

```text
root = *(EXE+0x064576F8)
node = *(root+0x10)
playerId = *(uint32_t*)(node+0x30)
```

This chain produced a valid runtime player ID in the October executable during the U80/U80F tests.

### Simplification rule

Do not reproduce U74's multiple player/readiness lanes.

The fresh resolver should only:

1. obtain the current player/loadout;
2. verify the expected type / basic validity;
3. obtain playerId;
4. return READY or NOT READY.

AUTO generation reset is based on this same resolved context.

---

## 2. ToggleLicenseToKill()

### Current native decision site

```text
EXE+0x191C344
```

October build OFF form:

```text
32 D2 4C 8B 15 53
xor dl,dl
```

October build ON form:

```text
B2 01 4C 8B 15 53
mov dl,1
```

The older build used R9B at the corresponding decision. The October update changed the register semantics to DL.

### Validation

This exact adaptation was exercised successfully in U80F:

```text
F1 License To Kill = ON
```

No weapon, ammo, AUTO or gadget state is required by this primitive.

The fresh implementation should keep one local boolean/current-byte state and switch only this validated decision sequence.

---

## 3. GiveWeapon(player, weapon)

### Design decision

No separate Q-Pistol engine is required.

The validated generic pair-clone path already handled:

- normal F5-F12 firearm RIDs;
- QPistolSilenced;
- QPistolUnsilenced.

Therefore:

```text
SwapQPistol(player)
    -> choose alternate Q-Pistol RID
    -> GiveWeapon(player, RID)
```

### Pair-clone model

Use one known donor graph:

```text
LightPistolNonLethal RID = 016886A4B599391C
```

Resolve:

- donor ItemEntry / Spawner;
- requested source ItemEntry / Spawner.

Temporarily replace the donor ItemEntry resolved descriptor with the requested weapon data.

Validated ItemEntry fields:

```text
ItemEntry+0x120 = weapon RID
ItemEntry+0x128 = resolved/cache/template qword
```

Both fields are restored after completion/failure.

### Current RTTI/vtable targets

```text
ZDynamicGameplaySpawnerItemEntryEntity = 0x142ECB538
ZDynamicGameplaySpawnerEntity          = 0x142ECC800
ZKntPlayerLoadoutEntity                = 0x142EDEB70
ZKntLoadoutCollectionEntity            = 0x142EDDFD8
```

### Native trigger

The gameplay-thread native trigger used by the validated pair-clone state is:

```text
EXE+0x16B6B10
```

The prepared donor spawner is passed as RCX.

The fresh core should expose this only behind:

```text
GiveWeapon(player, RID)
```

### Thread/serialization requirement

The native spawner is asynchronous.

A single minimal shared queue is permitted:

```text
weaponQueue[]
weaponBusy
currentRequest
```

Do not reproduce separate F4/AUTO/F5-F12 state machines.

One gameplay-thread hook/dispatcher may service this queue.

Full-function comparison identifies the current final gameplay hook point as:

```text
EXE+0x194D891
```

The short matching epilogue at `EXE+0x194CC91` is an intermediate look-alike and must not be used.

### October executable gameplay validation

U80F on the October executable successfully completed:

```text
F4 pair-clone weapon slot = 1
Native generic completed RID = 013F40B3AD756F14
F4 pair-clone weapon completion confirmed

F4 pair-clone weapon slot = 2
Native generic completed RID = 01E7EDCF4CA78A61
F4 pair-clone weapon completion confirmed
F4 native weapon set COMPLETE
```

Therefore the current vtables, ItemEntry offsets, native trigger and pair-clone concept are empirically valid on the new executable.

---

## 4. AddAmmo(player, profile)

### Critical architecture correction

Do **not** use the low-level current `ZCLSetFirearmAmmo` reserve-vector setter for Q Protocol's F2 action.

The October executable exposes the exact gameplay concept Q Protocol needs:

```text
ZCLGiveHumanoidPlayerAmmunition
Gameplay::SGpwInput_AddFirearmAmmunitionToPlayer
```

This means F2/AUTO can publish a native **add ammunition** input rather than reconstructing the game's internal reserve record.

### Native input record

The current gameplay input record is 12 bytes:

```cpp
struct AddFirearmAmmunitionToPlayer {
    uint32_t playerId;
    uint32_t amount;
    uint32_t firearmClass;
};
```

Reflection/native metadata for `ZCLGiveHumanoidPlayerAmmunition` confirms:

```text
m_ammunitionToGive         +0x28
m_overrideAmmunitionToGive +0x30
m_firearmClass             +0x40
```

### Current native publication path

Gameplay owner/global:

```text
EXE+0x064576E0
```

Relevant owner fields:

```text
lock/context        +0x238E0
staging/input vector +0x20B70
AddAmmo input pool   +0x20AD0
```

Helpers:

```text
generic vector insertion helper EXE+0x00116170
native publish helper            EXE+0x012A8FC0
EnterCriticalSection IAT         0x142B1F5E0
LeaveCriticalSection IAT         0x142B1F5D8
```

Native event:

```text
0x1DE
```

The engine's native sequence is:

```text
record = { playerId, amount, firearmClass }

lock owner+0x238E0
insert record into owner+0x20B70
publish event 0x1DE using owner+0x20AD0
unlock
```

### Why this is preferred

The separate `ZCLSetFirearmAmmo` path now builds a 24-byte identity/reference payload and publishes event 0x1DB.

That setter is lower-level and was the source of the misleading U80 ammo rebase work.

Q Protocol wants "ammo +", so `AddFirearmAmmunitionToPlayer` is the correct native abstraction.

### Shared profile implementation

```text
F2:
    AddAmmo(player, ManualAmmo)

AUTO:
    AddAmmo(player, AutoAmmo)
```

The function loops over configured classes and skips zero quantities.

No separate manual/Auto writer.

---

## 5. ApplyWeaponLoadout()

This is not a native primitive.

It is intentionally a thin Q Protocol helper:

```cpp
ApplyWeaponLoadout(player, profile) {
    for each enabled weapon in profile:
        GiveWeapon(player, weapon);
}
```

It uses the one shared weapon queue.

Manual:

```text
F3 -> ApplyWeaponLoadout(player, ManualLoadout)
```

AUTO:

```text
ApplyWeaponLoadout(player, AutoLoadout)
```

---

## 6. SwapQPistol()

Also intentionally thin:

```text
F4
 -> determine current/configured Q-Pistol mode
 -> choose QPistolSilenced or QPistolUnsilenced
 -> GiveWeapon(player, selected RID)
```

Known validated RIDs:

```text
QPistolSilenced   = 019973508A5E327B
QPistolUnsilenced = 01BE2C45AA55200D
```

Do not restore the old dedicated Q-Pistol state machine.

---

## Final fresh-core architecture

```text
ResolvePlayer()

F1 -> ToggleLicenseToKill()

F2 -> ResolvePlayer()
      AddAmmo(player, ManualAmmo)

F3 -> ResolvePlayer()
      ApplyWeaponLoadout(player, ManualLoadout)

F4 -> ResolvePlayer()
      SwapQPistol(player)

F5-F12
   -> ResolvePlayer()
      GiveWeapon(player, configuredWeapon)

AUTO
   -> ResolvePlayer()
      if new player generation:
          ApplyWeaponLoadout(player, AutoLoadout)
          AddAmmo(player, AutoAmmo)
          AutoDone = true
```

Only one weapon queue is shared by F3, F4, F5-F12 and AUTO.

## Audit verdict

The primitive audit is complete enough to begin the fresh implementation.

- ResolvePlayer: mapped.
- License To Kill: mapped and gameplay-validated.
- GiveWeapon: pair-clone/native trigger mapped and gameplay-validated on October executable.
- AddAmmo: current native AddFirearmAmmunition input path mapped.
- ApplyWeaponLoadout: thin shared helper.
- SwapQPistol: thin GiveWeapon wrapper.

The next step is **source implementation**, not another binary patch of U74/U80-U85.
