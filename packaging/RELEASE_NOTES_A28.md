# A28 November native remap TEST

Experimental compatibility port for Steam build **26252352**. The local update
is dated October 30; "November" names the development effort, not a claimed
publisher release date. **No runtime validation yet.**

Based on integrated A26/A27 commit `06245ba6623f590d165c05d227c078f2072831e5`.
Native RVAs are individually identified from LTK control flow, MSVC RTTI and
reflection type links, native ammo event `0x1DE`, spawn state fields and the
gameplay epilogue. The final gameplay callback still needs live confirmation.
PE gating is retained and augmented with code/type checks before any hook.

A25 UI, A26 renderer, A27 scanner, assembly, gameplay logic and INI remain frozen.
GPU DEVICE_LOST `0x887A002B` remains open. The old executable is unavailable.
This TEST targets EXE SHA-256
`6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1`.

The ZIP contains exactly `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt`
at root. Back up the installed mod and preserve your personal INI. See the
included README for rollback and the maintainer test sequence. Public v0.9.6
and all previous releases remain unchanged.
