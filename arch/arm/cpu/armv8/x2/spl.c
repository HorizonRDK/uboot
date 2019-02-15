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
#include <asm/arch/x2_dev.h>

#include "x2_info.h"
#include "x2_mmc_spl.h"
#include "x2_ap_spl.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_bootsrc __attribute__ ((section(".data"), aligned(8)));
struct x2_info_hdr g_binfo __attribute__ ((section(".data"), aligned(8)));
struct x2_dev_ops g_dev_ops;

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

static void spl_emmc_init(void)
{
#if 0
	dw_mmc_params_t params;

	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = SDIO0_BASE;
	params.bus_width = EMMC_BUS_WIDTH_8;
	params.clk_rate = EMMC_SDIO0_MCLK;
	params.sclk = EMMC_SDIO0_SCLK;
	params.flags = 0;

	emmc_init(&params);
#endif /* #if 0 */

	return;
}

void board_init_f(ulong dummy)
{
	preloader_console_init();

	spl_emmc_init();

	spl_ap_init();

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

#ifdef CONFIG_SPL_OS_BOOT

static void switch_to_el1(void)
{
#if 0
	writel(0x00000a0a, 0xA1006074);
	writel(0xbeadbeef, 0x10000000);
#endif /* #if 0 */

	armv8_switch_to_el1((u64)SPL_LOAD_DTB_ADDR, 0, 0, 0,
		SPL_LOAD_OS_ADDR,
		ES_TO_AARCH64);
}

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	spl_image->name = "Linux";
	spl_image->os = IH_OS_LINUX;
	spl_image->entry_point = (uintptr_t)switch_to_el1;
	spl_image->size = 0x800000;
	spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;

	return;
}

#endif /* CONFIG_SPL_OS_BOOT */
