# Optional board fragments (microsd)

## STM3210E-EVAL (CN13 SDIO)

| Item | Value |
|------|-------|
| Interface | SDIO (this driver uses 1-bit; the board wiring also exposes D1–D3) |
| D0–D3 / CLK | PC8–PC11 / PC12 |
| CMD | PD2 |
| Detect | PF11 (active low) |

### Jumpers (UM0455 Table 9)

| Jumper | For microSD | Notes |
|--------|-------------|-------|
| **JP17** | **Fitted** | Enables SD D0 (default fitted) |
| **JP20** | **Open** | Fitted disables JTAG/SWD; leave open |
| **JP22** | **Open** | Open → PC11 = SD D3; fitted → IrDA RX |

The current HAL driver initializes SDIO in **1-bit mode**; it does not switch
to 4-bit mode, so JP22 is not required for this sample's bus width. Keep JP17
fitted and JP20 open as above. Insert a card in CN13 before running the sample.
