# Optional board fragments (adc)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig for this sample only |
| `<board>.overlay` | Rare: map `zephyr,user` io-channels / pinmux |

Source code gates on `zephyr,user` `io-channels`, not `CONFIG_BOARD_*`.
The STM3210E-EVAL board DTS already provides channel 14 (RV1).

## STM3210E-EVAL (RV1)

| Item | Value |
|------|-------|
| Control | RV1 potentiometer |
| Pin | PC4 = ADC1_IN14 |
| Resolution | 12-bit (0–4095) |
| Vref | 3.3 V (board) |

Turn RV1 while the sample loops to see raw / mV change.
