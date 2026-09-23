# CAN — STM3210E-EVAL (UM0488 §2.7)

| Item | Detail |
|------|--------|
| PHY | SN65HVD230 → CN4 |
| MCU | CAN RX/TX **PB8/PB9** (remap1) |
| JP8 | high-speed (default) |
| JP6 | 120 Ω terminator (fit for bus) |

F103 **USB and CAN share IRQs 19/20** — this sample disables USB.

Runs in **loopback** (no external bus required).

```
west build -b stm3210e_eval -d build/stm3210e_eval/can samples/can -- \
  -DBOARD_ROOT=$ZPLATFORM_ROOT -DZEPHYR_EXTRA_MODULES=$ZPLATFORM_ROOT
```
