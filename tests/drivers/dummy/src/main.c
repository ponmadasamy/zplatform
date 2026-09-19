/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Placeholder driver-suite entry so Twister discovers tests/drivers/.
 * Replace with real out-of-tree driver tests as drivers are added.
 */

#include <zephyr/ztest.h>

ZTEST(zplatform_drivers_dummy, test_placeholder)
{
	zassert_true(true, "placeholder");
}

ZTEST_SUITE(zplatform_drivers_dummy, NULL, NULL, NULL, NULL, NULL);
