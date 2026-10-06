Q Protocol - Fresh Core A19Q Q-Pistol Native Bootstrap TEST
=============================================================

BASE
----
Strictly based on public v0.9.1 / Fresh Core A19 Audit Hardening.

ONE TESTED DIFFERENCE
---------------------
Recovered U84 shows the older October core kept the Q-Pistol on its own native
ItemEntry/Spawner path. Extra firearms used the donor pair-clone GiveWeapon.

Fresh Core A19 routed Q-Pistol through the same donor pair-clone.

A19Q restores only the old Q-Pistol distinction:
- QPistolSilenced / QPistolUnsilenced use their own native Spawner.
- OneHanded / TwoHanded / F5-F12 keep the existing A19 pair-clone.
- A19 queue order and 500 ms delay remain unchanged.
- A19 AUTO trigger/readiness remains unchanged.
- A19 F3 semantics remain unchanged.
- AddAmmo, RIDs, overlay and config remain unchanged.

IMPORTANT
---------
This test ZIP contains NO QProtocol.ini.
Keep your current v0.9.1 QProtocol.ini.

TEST
----
1. Replace only QProtocol.asi.
2. Verify normal gameplay.
3. Enter TacSim.
4. Press F3 once.
5. Restart/re-enter TacSim and test AUTO without pressing F3.
6. Send QProtocol.log.

Expected log:
Q-Pistol DIRECT prepared ...
Q-Pistol DIRECT COMPLETE ...
then normal GiveWeapon lines for OneHanded/TwoHanded.

Public release remains v0.9.1 / A19.
