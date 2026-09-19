/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <zplatform/info.h>
#include <zplatform/diag.h>

static int cmd_zplatform_info(const struct shell *sh, size_t argc, char **argv)
{
	struct zplatform_sys_info info;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (zplatform_get_sys_info(&info) != 0) {
		shell_error(sh, "failed to read system info");
		return -EIO;
	}

	shell_print(sh, "ZPlatform: %s", info.zplatform_version);
	shell_print(sh, "Zephyr:    %s", info.zephyr_version);
	shell_print(sh, "Board:     %s", info.board);
	shell_print(sh, "CPU:       %u Hz", info.cpu_freq_hz);
	shell_print(sh, "Flash:     %u", info.flash_size);
	shell_print(sh, "SRAM:      %u", info.sram_size);

	return 0;
}

static int cmd_zplatform_board(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%s", CONFIG_BOARD);
	return 0;
}

static int cmd_zplatform_memory(const struct shell *sh, size_t argc, char **argv)
{
	struct zplatform_mem_info mem;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (zplatform_get_mem_info(&mem) != 0) {
		shell_error(sh, "failed to read memory info");
		return -EIO;
	}

	shell_print(sh, "Flash used/total: %u / %u",
		    (unsigned int)mem.flash_used,
		    (unsigned int)mem.flash_total);
	shell_print(sh, "RAM used/total:   %u / %u (free %u)",
		    (unsigned int)mem.sram_used,
		    (unsigned int)mem.sram_total,
		    (unsigned int)mem.sram_free);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(zplatform_cmds,
	SHELL_CMD(info, NULL, "Show ZPlatform system information", cmd_zplatform_info),
	SHELL_CMD(board, NULL, "Show board name", cmd_zplatform_board),
	SHELL_CMD(memory, NULL, "Show memory usage", cmd_zplatform_memory),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(zplatform, &zplatform_cmds, "ZPlatform commands", NULL);
