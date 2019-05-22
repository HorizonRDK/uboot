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
#include "x2_nor_spl.h"
#include "x2_nand_spl.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_bootsrc __attribute__ ((section(".data"), aligned(8)));
struct x2_info_hdr g_binfo __attribute__ ((section(".data"), aligned(8)));
unsigned int g_x2ver __attribute__ ((section(".data"), aligned(8)));

struct x2_dev_ops g_dev_ops;

#if IS_ENABLED(CONFIG_TARGET_X2)
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>

extern struct dram_timing_info dram_timing;
#endif /* CONFIG_TARGET_X2 */

#if defined(CONFIG_X2_AP_BOOT)
unsigned int ap_boot_type = 1;
#endif

void spl_dram_init(void)
{
#if IS_ENABLED(CONFIG_TARGET_X2)
	/* ddr init */
	ddr_init(&dram_timing);
#endif /* CONFIG_TARGET_X2 */
}

void board_init_f(ulong dummy)
{
	if (g_x2ver == 0) {
		switch_peri_pll();
	}

	icache_enable();

	preloader_console_init();

#if defined(CONFIG_X2_AP_BOOT) && defined(CONFIG_TARGET_X2)
	spl_ap_init();
#elif defined(CONFIG_X2_YMODEM_BOOT)
	spl_x2_ymodem_init();
#elif defined(CONFIG_X2_MMC_BOOT)
	spl_emmc_init(g_binfo.emmc_cfg);
#elif defined(CONFIG_X2_NOR_BOOT)
	spl_nor_init();
#elif defined(CONFIG_X2_NAND_BOOT)
	spl_nand_init();
#endif

	spl_dram_init();

#if defined(CONFIG_X2_MMC_BOOT) || defined(CONFIG_X2_NOR_BOOT)
	/* write bootinfo to ddr 0x10000000 */
	x2_bootinfo_init();
#endif

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
#if defined(CONFIG_X2_AP_BOOT) \
	|| defined(CONFIG_X2_MMC_BOOT) \
	|| defined(CONFIG_X2_NOR_BOOT) \
	|| defined(CONFIG_X2_NAND_BOOT)

	return BOOT_DEVICE_RAM;
#elif defined(CONFIG_X2_YMODEM_BOOT)

	return BOOT_DEVICE_UART;
#endif
}

#ifdef CONFIG_SPL_OS_BOOT

#ifdef CONFIG_X2_AP_BOOT
static __maybe_unused void switch_to_el1(void)
{
	armv8_switch_to_el1((u64) SPL_LOAD_DTB_ADDR, 0, 0, 0,
			    SPL_LOAD_OS_ADDR, ES_TO_AARCH64);
}

#endif /* CONFIG_X2_AP_BOOT */

void spl_perform_fixups(struct spl_image_info *spl_image)
{
#if CONFIG_IS_ENABLED(LOAD_FIT)
	unsigned int fdt_size;

	if (spl_image->os == IH_OS_LINUX) {
		fdt_size = fdt_totalsize(spl_image->fdt_addr);

		spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;
		memcpy(spl_image->arg, spl_image->fdt_addr, fdt_size);
	}
#elif defined(CONFIG_X2_AP_BOOT)
	spl_image->name = "Linux";
	spl_image->os = IH_OS_LINUX;
	spl_image->entry_point = (uintptr_t)switch_to_el1;
	spl_image->size = 0x800000;
	spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;
#endif /* CONFIG_SPL_LOAD_FIT */

	return;
}

#endif /* CONFIG_SPL_OS_BOOT */

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif /* CONFIG_SPL_LOAD_FIT */
