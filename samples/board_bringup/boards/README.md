# Optional board fragments (board_bringup)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig for this sample on that board only |
| `<board>.overlay` | Rare: enable a board node disabled by default |

Pin mux and aliases belong in `boards/<vendor>/<board>/`, not here.
