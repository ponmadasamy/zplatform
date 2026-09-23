# Optional board fragments (ext_sram)

## STM3210E-EVAL

| Item | Value |
|------|-------|
| Device | IS61WV51216BLL-10MLI |
| Bus | FSMC bank1 NOR/PSRAM3 |
| Base | `0x68000000` |
| Size | 512K×16 = 1 MiB |
| CS | NE3 (PG10) |

No jumpers required beyond a powered board. Shares the FSMC data bus
with the MB895 LCD (bank4).
