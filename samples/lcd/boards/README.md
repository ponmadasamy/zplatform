# Optional board fragments (lcd)

Source uses `DEVICE_DT_GET(DT_CHOSEN(zephyr_display))` and the Zephyr
`display` API (`display_write`, `display_blanking_off`).

## STM3210E-EVAL (MB895)

| Item | Value |
|------|-------|
| Module | MB895 on **CN16** |
| Driver | `zplatform,mb895-lcd` → FSMC bank4 |
| Controllers | HX8347D / SPFD5408 / ILI9320 (runtime ID) |
| Size | 320×240 RGB565 (Cube landscape) |

Expect red top half + blue bottom half after blanking_off.
