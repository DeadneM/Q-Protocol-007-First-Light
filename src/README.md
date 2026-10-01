# Fresh Core source

This directory is the new Q Protocol implementation.

It is intentionally **not** a source reconstruction of U74/U80-U85.

## Fresh Core A1

A1 implements only:

- ASI/DLL bootstrap;
- launch log truncation;
- target executable validation;
- one shared `ResolvePlayer()`;
- F1 License To Kill toggle.

F2-F12, AUTO, weapons, ammo and overlay are intentionally inactive in A1.

This gives us the smallest possible runtime proof that the new source can load, resolve the player and perform the isolated LTK action safely.

See:

- `/PROJECT_STATE.md`
- `/docs/PRIMITIVE_AUDIT_OCT2026.md`
- `/docs/OVERLAY_PLAN.md`
