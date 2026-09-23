# USB CDC — STM3210E-EVAL (UM0488 §2.15)

| Item | Detail |
|------|--------|
| Connector | CN14 Type-B |
| Pins | PA11/PA12 |
| JP13 | USB power path if using CN14 for 5 V |

Do **not** enable CAN in the same image (shared IRQs).

```
west build -b stm3210e_eval -d build/stm3210e_eval/usb_cdc samples/usb_cdc -- \
  -DBOARD_ROOT=$ZPLATFORM_ROOT -DZEPHYR_EXTRA_MODULES=$ZPLATFORM_ROOT
```

Host: plug CN14, open the new CDC COM port, type — characters echo.
