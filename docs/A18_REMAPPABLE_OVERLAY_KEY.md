# Fresh Core A18 — Remappable Overlay Key

A18 removes the last hardcoded Insert dependency from the overlay toggle path.

## Configuration

```ini
[Overlay]
Enabled=1
ToggleKey=Insert
```

`ToggleKey` is loaded at overlay initialization and on Reload.

## In-game remap

The Hotkeys tab includes an **Overlay / Menu toggle key** control.

1. Click the current key.
2. Press the desired keyboard key.
3. Click Save.

Escape cancels capture.

## Safety

- missing or invalid values fall back to Insert;
- modifier-only keys are ignored;
- mouse buttons are ignored;
- the newly captured key is edge-latched so the capture press does not
  immediately close the overlay;
- readable names are stored for common keys;
- unknown valid virtual keys are preserved as `VK_XX`.

## Scope

No gameplay primitive changed. A17 loadout behavior and catalogue state are
preserved.
