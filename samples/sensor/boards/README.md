# Optional board fragments (sensor)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig (e.g. `CONFIG_LM75=y`) for this sample only |
| `<board>.overlay` | Rare: enable a sensor node disabled by default |

Source code gates on `DT_HAS_COMPAT_STATUS_OKAY` / `DT_NODE_EXISTS`, not
`CONFIG_BOARD_*`. Sensor nodes themselves live in the board DTS.
