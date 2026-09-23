# RTC — STM3210E-EVAL (UM0488 §2.3)

X2 32 kHz / LSI. On **STM32F1**, Zephyr exposes RTC as a **counter** (no
`rtc_get_time` calendar driver).

```
west build -b stm3210e_eval -d build/stm3210e_eval/rtc samples/rtc -- \
  -DBOARD_ROOT=$ZPLATFORM_ROOT -DZEPHYR_EXTRA_MODULES=$ZPLATFORM_ROOT
```
