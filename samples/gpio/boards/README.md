# Optional board fragments (gpio)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig for this sample on that board only |
| `<board>.overlay` | Rare: enable a board node disabled by default |

Pin mux and aliases belong in `boards/<vendor>/<board>/`, not here.

## STM3210E-EVAL

`boards/stm3210e_eval.conf` enables LED triggers + power-off.

| Input | Action |
|-------|--------|
| User Key (B4) | Pause / resume all LED patterns |
| Tamper (B3) | Enter STANDBY (power off) |
| Wakeup (B2) | WKUP on PA0 → chip reset → full boot (JP4 open) |
| Joystick Up / Down | Timer pulse gap + / − |
| Joystick Right / Left | Rotate triggers across LD1–LD4 |

Default LED map: **LD1** heartbeat, **LD2** cpu, **LD3** timer, **LD4** panic.
