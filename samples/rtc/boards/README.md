# RTC — STM3210E-EVAL (UM0488 §2.3)

The board DTS currently selects **LSI** for the RTC. Although the board has an
X2 32 kHz crystal and CR1220 backup battery, this sample does not configure or
verify battery-backed time retention. On **STM32F1**, Zephyr exposes RTC as a
**counter** (no `rtc_get_time` calendar driver).

```
west build -b stm3210e_eval -d build/stm3210e_eval/rtc samples/rtc -- \
  -DBOARD_ROOT=$ZPLATFORM_ROOT -DZEPHYR_EXTRA_MODULES=$ZPLATFORM_ROOT
```
