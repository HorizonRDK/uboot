/*
 * Copyright 2015 - 2016 Xilinx, Inc.
 *
 * Michal Simek <michal.simek@xilinx.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <spl.h>

#include <asm/io.h>
#include <asm/spl.h>

#include "x2_info.h"
#include "x2_mmc_spl.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_bootsrc __attribute__ ((section(".data"), aligned(8)));
struct x2_info_hdr g_binfo __attribute__ ((section(".data"), aligned(8)));

#ifdef CONFIG_TARGET_X2
#include <asm/arch/ddr.h>

extern struct dram_timing_info dram_timing;
#endif /* CONFIG_TARGET_X2 */

void spl_dram_init(void)
{
#ifdef CONFIG_TARGET_X2
	/* ddr init */
	ddr_init(&dram_timing);
#endif /* CONFIG_TARGET_X2 */
}

void board_init_f(ulong dummy)
{
	preloader_console_init();

	spl_dram_init();

	return;
}

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void)
{
	return;
}
#endif /* CONFIG_SPL_BOARD_INIT */

unsigned int spl_boot_device(void)
{
	return BOOT_DEVICE_RAM;
}

unsigned int x2_boot_from_emmc(uint64_t load_addr)
{
	dw_mmc_params_t params;

	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = X2_SDIO0_BASE;
	params.bus_width = EMMC_BUS_WIDTH_8;
	params.clk_rate = EMMC_SDIO0_MCLK;
	params.sclk = EMMC_SDIO0_SCLK;
	params.flags = 0;

	hobot_emmc_init(&params);
	//emmc_read_blocks(blk_num, load_addr, bytes_num);
	return 0;
}
