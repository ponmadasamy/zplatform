# DAC — STM3210E-EVAL (UM0488 §2.5)

MCU DAC1 channel 1 on **PA4** (audio / expansion). Channel 2 (PA5) is SPI1 SCK.

Probe PA4 / CN10 with a scope while the sawtooth steps.

```
west build -b stm3210e_eval -d build/stm3210e_eval/dac samples/dac -- \
  -DBOARD_ROOT=$ZPLATFORM_ROOT -DZEPHYR_EXTRA_MODULES=$ZPLATFORM_ROOT
```
