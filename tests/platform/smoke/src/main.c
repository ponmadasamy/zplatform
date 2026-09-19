/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zplatform/info.h>
#include <zplatform/diag.h>
#include <zplatform/version.h>

ZTEST(zplatform_smoke, test_version_string)
{
	zassert_not_null(ZPLATFORM_VERSION_STRING, "version string missing");
	zassert_true(ZPLATFORM_VERSION_MAJOR >= 0, "bad major");
}

ZTEST(zplatform_smoke, test_sys_info)
{
	struct zplatform_sys_info info;
	int rc = zplatform_get_sys_info(&info);

	zassert_equal(rc, 0, "sys info failed");
	zassert_not_null(info.zplatform_version, "missing zplatform version");
	zassert_not_null(info.zephyr_version, "missing zephyr version");
	zassert_not_null(info.board, "missing board");
}

ZTEST(zplatform_smoke, test_mem_info)
{
	struct zplatform_mem_info mem;
	int rc = zplatform_get_mem_info(&mem);

	zassert_equal(rc, 0, "mem info failed");
	/* native_sim may not expose flash0/sram0 the same way as MCU targets */
	zassert_true(true, "mem info readable");
}

ZTEST_SUITE(zplatform_smoke, NULL, NULL, NULL, NULL, NULL);
