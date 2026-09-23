# Optional board fragments (pwm)

Zephyr auto-merges these when present:

| File | Use |
|------|-----|
| `<board>.conf` | Extra Kconfig for this sample only |
| `<board>.overlay` | Rare: map `pwm-led0` / pinmux |

Source code gates on alias `pwm-led0`, not `CONFIG_BOARD_*`.
The STM3210E-EVAL board DTS already provides TIM1_CH1 on PA8.

## STM3210E-EVAL (TIM1_CH1)

| Item | Value |
|------|-------|
| Timer | TIM1 channel 1 |
| Pin | PA8 |
| Probe | CN10 / motor-control header (no on-board LED) |
| Default period | 20 ms (50 Hz), 50 % duty |

Scope PA8 vs GND to confirm the waveform; console reports PASS on `pwm_set`.
