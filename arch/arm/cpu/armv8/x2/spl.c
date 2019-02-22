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
#include "x2_ymodem_spl.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_bootsrc __attribute__ ((section(".data"), aligned(8)));
struct x2_info_hdr g_binfo __attribute__ ((section(".data"), aligned(8)));
struct x2_dev_ops g_dev_ops;

#if IS_ENABLED(CONFIG_TARGET_X2)
#include <asm/arch/ddr.h>

extern struct dram_timing_info dram_timing;
#endif /* CONFIG_TARGET_X2 */

void spl_dram_init(void)
{
#if IS_ENABLED(CONFIG_TARGET_X2)
	/* ddr init */
	ddr_init(&dram_timing);
#endif /* CONFIG_TARGET_X2 */
}

#if 0
static void spl_emmc_init(void)
{
	dw_mmc_params_t params;

	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = SDIO0_BASE;
	params.bus_width = EMMC_BUS_WIDTH_8;
	params.clk_rate = EMMC_SDIO0_MCLK;
	params.sclk = EMMC_SDIO0_SCLK;
	params.flags = 0;

	emmc_init(&params);

	return;
}
#endif /* #if 0 */

void board_init_f(ulong dummy)
{
	preloader_console_init();

#if defined(CONFIG_X2_AP_BOOT) && defined(CONFIG_TARGET_X2)
	spl_ap_init();
#elif defined(CONFIG_X2_YMODEM_BOOT)
	spl_x2_ymodem_init();

#ifdef CONFIG_TARGET_X2_FPGA
	printf("\nLoad ddr imem 1d ...\n");
	g_dev_ops.read(0, 0x80007000, 0);

	printf("\nLoad ddr dmem 1d ...\n");
	g_dev_ops.read(1, 0x80007000, 0);

	printf("\nLoad ddr imem 2d ...\n");
	g_dev_ops.read(2, 0x80007000, 0);

	printf("\nLoad ddr dmem 2d ...\n");
	g_dev_ops.read(3, 0x80007000, 0);
#endif /* CONFIG_TARGET_X2_FPGA */
#elif defined(CONFIG_X2_MMC_BOOT)
	spl_emmc_init();
#endif

	spl_dram_init();

	return;
}

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void)
{
#if IS_ENABLED(CONFIG_TARGET_X2)
	cnn_pll_init();

	vio_pll_init();
#endif /* CONFIG_TARGET_X2 */

	return;
}
#endif /* CONFIG_SPL_BOARD_INIT */

unsigned int spl_boot_device(void)
{
#if defined(CONFIG_X2_AP_BOOT)

	return BOOT_DEVICE_RAM;
#elif defined(CONFIG_X2_YMODEM_BOOT)

	return BOOT_DEVICE_UART;
#endif
}

#ifdef CONFIG_SPL_OS_BOOT

static void switch_to_el1(void)
{
	armv8_switch_to_el1((u64)SPL_LOAD_DTB_ADDR, 0, 0, 0,
		SPL_LOAD_OS_ADDR,
		ES_TO_AARCH64);
}

#ifdef CONFIG_X2_AP_BOOT
void spl_perform_fixups(struct spl_image_info *spl_image)
{
	spl_image->name = "Linux";
	spl_image->os = IH_OS_LINUX;
	spl_image->entry_point = (uintptr_t)switch_to_el1;
	spl_image->size = 0x800000;
	spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;

	return;
}
#endif /* CONFIG_X2_AP_BOOT */

#endif /* CONFIG_SPL_OS_BOOT */

