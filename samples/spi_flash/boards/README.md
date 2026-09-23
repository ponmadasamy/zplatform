# Optional board fragments (spi_flash)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig for this sample only |
| `<board>.overlay` | Rare: enable a flash node disabled by default |

Source code gates on `DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)`, not
`CONFIG_BOARD_*`. The SPI NOR node lives in the board DTS.

## STM3210E-EVAL (M25P64)

| Item | Value |
|------|-------|
| Part | M25P64-VME6G (U6), 64 Mbit |
| Bus | SPI1 — PA5 SCK, PA6 MISO, PA7 MOSI |
| CS | PB2 |
| Jumper | **JP3** must select serial Flash MISO (UM0455) |

Expected JEDEC ID: `20 20 17`.
