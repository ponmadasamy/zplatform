# Optional board fragments (lvgl)

## STM3210E-EVAL

Uses `zephyr,display` → MB895 (320×240 RGB565) and places LVGL VDB + heap
in **EXT_SRAM** (`0x68000000`, FSMC bank3) via:

```
CONFIG_LV_Z_VDB_ZEPHYR_REGION=y
CONFIG_LV_Z_VDB_ZEPHYR_REGION_NAME="EXT_SRAM"
```

FSMC bank3 is brought up by `SYS_INIT` in `lib/fsmc/fsmc_sram.c` before
LVGL auto-init. Expect a dark splash with **ZPlatform** on the TFT.
