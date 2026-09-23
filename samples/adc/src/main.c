/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic ADC sample. Uses the first io-channel under zephyr,user
 * when present (RV1 / ADC1_IN14 on STM3210E-EVAL).
 */

#include <inttypes.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define HAS_ADC_IO_CHANNELS 1
#else
#define HAS_ADC_IO_CHANNELS 0
#endif

int main(void)
{
	printk("ADC sample\n");

#if !HAS_ADC_IO_CHANNELS
	printk("[SKIP] No zephyr,user io-channels in DeviceTree\n");
	return 0;
#else
	static const struct adc_dt_spec adc_chan =
		ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
	uint16_t sample_buf = 0;
	struct adc_sequence sequence = {
		.buffer = &sample_buf,
		.buffer_size = sizeof(sample_buf),
	};
	int err;

	if (!adc_is_ready_dt(&adc_chan)) {
		printk("[FAIL] ADC device not ready\n");
		return 0;
	}

	err = adc_channel_setup_dt(&adc_chan);
	if (err < 0) {
		printk("[FAIL] ADC channel setup (%d)\n", err);
		return 0;
	}

	printk("[PASS] ADC ready (%s ch%d)\n",
	       adc_chan.dev->name, adc_chan.channel_id);

	while (1) {
		int32_t val_mv;

		(void)adc_sequence_init_dt(&adc_chan, &sequence);
		err = adc_read_dt(&adc_chan, &sequence);
		if (err < 0) {
			printk("[FAIL] ADC read (%d)\n", err);
		} else {
			val_mv = sample_buf;
			err = adc_raw_to_millivolts_dt(&adc_chan, &val_mv);
			if (err < 0) {
				printk("RV1 raw=%" PRIu16 " (mV n/a)\n",
				       sample_buf);
			} else {
				printk("RV1 raw=%" PRIu16 " (%" PRId32 " mV)\n",
				       sample_buf, val_mv);
			}
		}
		k_msleep(500);
	}
#endif

	return 0;
}
