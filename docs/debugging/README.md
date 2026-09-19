# Debugging ZPlatform

## Recommended flow

```text
ST-Link (SWD)
   → OpenOCD / J-Link GDB server
   → arm-zephyr-eabi-gdb
   → zephyr.elf
```

```bash
./scripts/sh/build.sh demo --board stm3210e_eval --debug
./scripts/sh/debug.sh
```

## GDB essentials

```text
break main
continue
step / next / finish
info registers
x/16xw $sp
bt
info threads
thread apply all bt
monitor reset halt
monitor reset run
```

Watchpoints (hardware limited on Cortex-M3):

```text
watch variable_name
```

## VS Code / Cursor

Use a `launch.json` that points GDB at:

```text
build/<board>/<app>/<cfg>/zephyr/zephyr.elf
```

and starts OpenOCD with `boards/st/stm3210e_eval/support/openocd.cfg`,
or simply rely on `west debug` from the integrated terminal.

## Runners

| Runner | When |
|--------|------|
| openocd | Default for STM3210E-EVAL |
| stm32cubeprogrammer | ST utilities installed |
| jlink | SEGGER probe |

```bash
./scripts/sh/flash.sh --runner openocd
./scripts/sh/debug.sh --runner jlink
```
