# Optional board fragments (microsd)

## STM3210E-EVAL (CN13 SDIO)

| Item | Value |
|------|-------|
| Interface | SDIO (1-bit always works; 4-bit if JP22 open) |
| D0–D3 / CLK | PC8–PC11 / PC12 |
| CMD | PD2 |
| Detect | PF11 (active low) |

### Jumpers (UM0455 Table 9)

| Jumper | For microSD | Notes |
|--------|-------------|-------|
| **JP17** | **Fitted** | Enables SD D0 (default fitted) |
| **JP20** | **Open** | Fitted disables JTAG/SWD; leave open |
| **JP22** | **Open** | Open → PC11 = SD D3; fitted → IrDA RX |

Driver uses **1-bit** SDIO for bring-up (reliable if JP22 is fitted for IrDA).
Insert a card in CN13 before flashing.
