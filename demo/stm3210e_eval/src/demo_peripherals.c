/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

#include <zplatform/mb895_lcd.h>
#include <zplatform/fsmc_sram.h>
#include <zplatform/sdio_sd.h>

#include "demo_internal.h"

#if DT_HAS_CHOSEN(zephyr_canbus)
CAN_MSGQ_DEFINE(demo_can_rx, 2);
#endif

void demo_run_peripherals(void)
{
	printk("Peripherals\n");

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	{
		const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

		if (device_is_ready(i2c)) {
			printk(" [PASS] I2C\n");
		} else {
			printk(" [FAIL] I2C - not ready\n");
		}
	}
#else
	printk(" [SKIP] I2C - not available\n");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
	{
		const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi1));

		if (device_is_ready(spi)) {
			printk(" [PASS] SPI\n");
		} else {
			printk(" [FAIL] SPI - not ready\n");
		}
	}
#else
	printk(" [SKIP] SPI - not available\n");
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
	{
		const struct device *flash =
			DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor));
		uint8_t id[3] = {0};

		if (!device_is_ready(flash)) {
			printk(" [FAIL] SPI NOR - not ready\n");
		} else if (flash_read_jedec_id(flash, id) != 0) {
			printk(" [FAIL] SPI NOR - JEDEC read-ID\n");
		} else {
			printk(" [PASS] SPI NOR JEDEC %02x %02x %02x\n",
			       id[0], id[1], id[2]);
		}
	}
#else
	printk(" [SKIP] SPI NOR - not in DT\n");
#endif

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	{
		static const struct adc_dt_spec adc_chan =
			ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
		uint16_t sample_buf = 0;
		struct adc_sequence sequence = {
			.buffer = &sample_buf,
			.buffer_size = sizeof(sample_buf),
		};
		int err;
		int32_t val_mv;

		if (!adc_is_ready_dt(&adc_chan)) {
			printk(" [FAIL] ADC - not ready\n");
		} else if ((err = adc_channel_setup_dt(&adc_chan)) < 0) {
			printk(" [FAIL] ADC - channel setup (%d)\n", err);
		} else if ((err = adc_sequence_init_dt(&adc_chan, &sequence)) < 0) {
			printk(" [FAIL] ADC - sequence init (%d)\n", err);
		} else if ((err = adc_read_dt(&adc_chan, &sequence)) < 0) {
			printk(" [FAIL] ADC - read (%d)\n", err);
		} else {
			val_mv = sample_buf;
			if (adc_raw_to_millivolts_dt(&adc_chan, &val_mv) < 0) {
				printk(" [PASS] ADC ch%d raw=%u\n",
				       adc_chan.channel_id, sample_buf);
			} else {
				printk(" [PASS] ADC ch%d raw=%u (%d mV)\n",
				       adc_chan.channel_id, sample_buf, val_mv);
			}
		}
	}
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)
	{
		const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc1));

		if (device_is_ready(adc)) {
			printk(" [PASS] ADC (ready; no io-channels)\n");
		} else {
			printk(" [FAIL] ADC - not ready\n");
		}
	}
#else
	printk(" [SKIP] ADC - not available\n");
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
	{
		static const struct pwm_dt_spec pwm =
			PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
		const uint32_t period = PWM_MSEC(20);
		const uint32_t pulse = period / 2U;
		int err;

		if (!pwm_is_ready_dt(&pwm)) {
			printk(" [FAIL] PWM - not ready\n");
		} else if ((err = pwm_set_dt(&pwm, period, pulse)) != 0) {
			printk(" [FAIL] PWM - set (%d)\n", err);
		} else {
			printk(" [PASS] PWM ch%d %u/%u ns\n",
			       pwm.channel, pulse, period);
		}
	}
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
	{
		const struct device *pwm = DEVICE_DT_GET(DT_NODELABEL(pwm1));

		if (device_is_ready(pwm)) {
			printk(" [PASS] PWM (ready; no pwm-led0)\n");
		} else {
			printk(" [FAIL] PWM - not ready\n");
		}
	}
#else
	printk(" [SKIP] PWM - not available\n");
#endif

#if DT_HAS_CHOSEN(zephyr_display)
	{
		const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

		if (!device_is_ready(disp)) {
			printk(" [FAIL] LCD - display not ready\n");
#if defined(CONFIG_LVGL)
		} else {
#if defined(CONFIG_ZPLATFORM_MB895_LCD)
			printk(" [PASS] LCD ID 0x%04x (%s) (LVGL owns panel)\n",
			       zplatform_mb895_lcd_id(),
			       zplatform_mb895_lcd_id_name(
				       zplatform_mb895_lcd_id()));
#else
			printk(" [PASS] LCD ready (LVGL owns panel)\n");
#endif
		}
#else /* !CONFIG_LVGL */
		} else {
			struct display_capabilities caps;
			struct display_buffer_descriptor desc;
			uint16_t line[ZPLATFORM_MB895_LCD_WIDTH];
			uint16_t color = ZPLATFORM_RGB565(0, 90, 180);
			int rc;
			bool ok = true;

			display_get_capabilities(disp, &caps);
			for (uint16_t i = 0; i < caps.x_resolution; i++) {
				line[i] = color;
			}
			desc.buf_size = sizeof(line);
			desc.width = caps.x_resolution;
			desc.height = 1;
			desc.pitch = caps.x_resolution;
			desc.frame_incomplete = false;

			for (uint16_t y = 0; y < caps.y_resolution; y++) {
				rc = display_write(disp, 0, y, &desc, line);
				if (rc < 0) {
					printk(" [FAIL] LCD display_write (%d)\n",
					       rc);
					ok = false;
					break;
				}
			}
			if (ok && display_blanking_off(disp) < 0) {
				printk(" [FAIL] LCD blanking_off\n");
				ok = false;
			}
			if (ok) {
#if defined(CONFIG_ZPLATFORM_MB895_LCD)
				printk(" [PASS] LCD ID 0x%04x (%s) display_write\n",
				       zplatform_mb895_lcd_id(),
				       zplatform_mb895_lcd_id_name(
					       zplatform_mb895_lcd_id()));
#else
				printk(" [PASS] LCD display_write %ux%u\n",
				       caps.x_resolution, caps.y_resolution);
#endif
			}
		}
#endif /* CONFIG_LVGL */
	}
#else
	printk(" [SKIP] LCD - no zephyr,display\n");
#endif

#if defined(CONFIG_ZPLATFORM_FSMC_SRAM)
	{
		int rc = zplatform_fsmc_sram_init();

		if (rc != 0) {
			printk(" [FAIL] FSMC SRAM init (%d)\n", rc);
#if defined(CONFIG_LVGL)
		} else {
			/* Selftest would wipe LVGL VDB/heap in EXT_SRAM. */
			printk(" [PASS] FSMC SRAM @ 0x%08x (LVGL buffers)\n",
			       ZPLATFORM_FSMC_SRAM_BASE);
#else
		} else if (zplatform_fsmc_sram_selftest(4096U) != 0) {
			printk(" [FAIL] FSMC SRAM selftest\n");
		} else {
			printk(" [PASS] FSMC SRAM @ 0x%08x selftest 4 KiB\n",
			       ZPLATFORM_FSMC_SRAM_BASE);
#endif
		}
	}
#else
	printk(" [SKIP] FSMC SRAM - not enabled\n");
#endif

#if defined(CONFIG_ZPLATFORM_SDIO_SD)
	{
		struct zplatform_sd_info info;
		uint8_t blk[ZPLATFORM_SD_BLOCK_SIZE] __aligned(4);
		int rc;

		if (!zplatform_sdio_sd_detected()) {
			printk(" [SKIP] microSD - not detected\n");
		} else if ((rc = zplatform_sdio_sd_init()) != 0) {
			printk(" [FAIL] microSD init (%d)\n", rc);
		} else if (zplatform_sdio_sd_get_info(&info) != 0) {
			printk(" [FAIL] microSD get_info\n");
		} else if (zplatform_sdio_sd_read(0, blk, 1) != 0) {
			printk(" [FAIL] microSD read LBA0\n");
		} else {
			printk(" [PASS] microSD %u blocks read LBA0\n",
			       info.log_block_nbr);
		}
	}
#else
	printk(" [SKIP] microSD - not enabled\n");
#endif

#if FIXED_PARTITION_EXISTS(storage_partition)
	printk(" [PASS] Flash (storage partition present)\n");
#elif defined(CONFIG_FLASH)
	printk(" [PASS] Flash (CONFIG_FLASH enabled)\n");
#else
	printk(" [SKIP] Flash - not available\n");
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(lm75)
	{
		const struct device *dev =
			DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(lm75));
		struct sensor_value val;

		if (device_is_ready(dev) &&
		    sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val) == 0) {
			printk(" [PASS] Temperature sensor (%d.%06d C)\n",
			       val.val1, val.val2);
		} else if (device_is_ready(dev)) {
			printk(" [PASS] Temperature sensor (device ready; read needs HW)\n");
		} else {
			printk(" [FAIL] Temperature sensor\n");
		}
	}
#else
	printk(" [SKIP] Temperature sensor - not in DT\n");
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(die_temp0), okay)
	{
		const struct device *dev = DEVICE_DT_GET(DT_ALIAS(die_temp0));
		struct sensor_value val;

		if (device_is_ready(dev) &&
		    sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val) == 0) {
			printk(" [PASS] Die temp (%d.%06d C)\n", val.val1,
			       val.val2);
		} else {
			printk(" [FAIL] Die temp\n");
		}
	}
#else
	printk(" [SKIP] Die temp - not in DT\n");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rtc), okay)
	{
		const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
		uint32_t ticks = 0;
		int rc;

		if (!device_is_ready(rtc)) {
			printk(" [FAIL] RTC counter - not ready\n");
		} else if ((rc = counter_start(rtc)) < 0 && rc != -EALREADY) {
			printk(" [FAIL] RTC start (%d)\n", rc);
		} else if (counter_get_value(rtc, &ticks) != 0) {
			printk(" [FAIL] RTC get_value\n");
		} else {
			printk(" [PASS] RTC counter ticks=%u freq=%u\n", ticks,
			       counter_get_frequency(rtc));
		}
	}
#else
	printk(" [SKIP] RTC - not in DT\n");
#endif

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dac)
	{
		const struct device *dac =
			DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), dac));
		const uint8_t ch =
			DT_PROP(DT_PATH(zephyr_user), dac_channel_id);
		struct dac_channel_cfg cfg = {
			.channel_id = ch,
			.resolution = DT_PROP(DT_PATH(zephyr_user),
					      dac_resolution),
			.buffered = true,
		};

		if (!device_is_ready(dac)) {
			printk(" [FAIL] DAC - not ready\n");
		} else if (dac_channel_setup(dac, &cfg) != 0) {
			printk(" [FAIL] DAC - setup\n");
		} else if (dac_write_value(dac, ch, 2048) != 0) {
			printk(" [FAIL] DAC - write\n");
		} else {
			printk(" [PASS] DAC ch%u mid-scale\n", ch);
		}
	}
#else
	printk(" [SKIP] DAC - not in DT\n");
#endif

#if DT_HAS_CHOSEN(zephyr_canbus)
	{
		const struct device *can =
			DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
		struct can_frame tx = {
			.id = 0x321,
			.dlc = 1,
			.data = {0x42},
		};
		const struct can_filter filt = {
			.flags = 0,
			.id = 0x321,
			.mask = CAN_STD_ID_MASK,
		};
		struct can_frame rx;
		int fid;
		int rc;

		if (!device_is_ready(can)) {
			printk(" [FAIL] CAN - not ready\n");
		} else if ((rc = can_set_mode(can, CAN_MODE_LOOPBACK)) != 0) {
			printk(" [FAIL] CAN mode (%d)\n", rc);
		} else if ((rc = can_start(can)) != 0) {
			printk(" [FAIL] CAN start (%d)\n", rc);
		} else if ((fid = can_add_rx_filter_msgq(can, &demo_can_rx,
							 &filt)) < 0) {
			printk(" [FAIL] CAN filter (%d)\n", fid);
		} else if ((rc = can_send(can, &tx, K_MSEC(100), NULL, NULL)) !=
			   0) {
			printk(" [FAIL] CAN send (%d)\n", rc);
		} else if (k_msgq_get(&demo_can_rx, &rx, K_MSEC(100)) != 0) {
			printk(" [FAIL] CAN RX timeout\n");
		} else {
			printk(" [PASS] CAN loopback id=0x%X\n", rx.id);
		}
	}
#else
	printk(" [SKIP] CAN - not chosen (see samples/can)\n");
#endif

	printk(" [SKIP] USB CDC - samples/usb_cdc (IRQ share with CAN)\n");
	printk(" [SKIP] FSMC NAND/NOR / Audio AK4343 / Smartcard / IrDA\n");
	printk("\n");
}
