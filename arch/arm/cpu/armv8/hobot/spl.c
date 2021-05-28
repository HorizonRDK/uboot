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
#include <asm/arch/hb_dev.h>
#include <asm/arch/hb_pmu.h>
#include <veeprom.h>

#if defined(CONFIG_HB_WATCHDOG)
#include <watchdog.h>
#endif

#include "hb_info.h"
#include "hb_mmc_spl.h"
#include "hb_ap_spl.h"
#include "hb_ymodem_spl.h"
#include "hb_nor_spl.h"
#include "hb_nand_spl.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_bootsrc __attribute__ ((section(".data"), aligned(8)));
struct hb_info_hdr g_binfo __attribute__ ((section(".data"), aligned(8)));
unsigned int g_romver __attribute__ ((section(".data"), aligned(8)));

struct hb_dev_ops g_dev_ops;

#if IS_ENABLED(CONFIG_TARGET_X2)
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>

extern struct dram_timing_info dram_timing;
#endif /* CONFIG_TARGET_X2 */

#if defined(CONFIG_HB_AP_BOOT)
unsigned int ap_boot_type = 1;
#endif

void spl_dram_init(void)
{
#if IS_ENABLED(CONFIG_TARGET_X2)
	/* ddr init */
	ddr_init(&dram_timing);
#endif /* CONFIG_TARGET_X2 */
}

/* GPIO PIN MUX */
#define PIN_MUX_BASE    0xA6003000
#define GPIO1_CFG (PIN_MUX_BASE + 0x10)

static int bif_change_reset2gpio(void)
{
	unsigned int reg_val;

	/*set gpio1[15] GPIO function*/
	reg_val = readl(GPIO1_CFG);
	reg_val |= 0xc0000000;
	writel(reg_val, GPIO1_CFG);
	return 0;
}

void board_init_f(ulong dummy)
{
	char pllswitch[VEEPROM_PERI_PLL_SIZE] = { 0 };

	bif_change_reset2gpio();
	writel(0xFED00000, BIF_SHARE_REG_BASE);
	icache_enable();

	preloader_console_init();

#if defined(CONFIG_HB_AP_BOOT) && defined(CONFIG_TARGET_X2)
	spl_ap_init();
#elif defined(CONFIG_HB_YMODEM_BOOT)
	spl_hb_ymodem_init();
#elif defined(HB_MMC_BOOT)
	spl_emmc_init(g_binfo.emmc_cfg);
#elif defined(HB_NOR_BOOT)
	spl_nor_init(g_binfo.qspi_cfg);
#elif defined(HB_NAND_BOOT)
	spl_nand_init();
	udelay(40);
#endif

#if defined(HB_MMC_BOOT)
	veeprom_read(VEEPROM_PERI_PLL_OFFSET, pllswitch,VEEPROM_PERI_PLL_SIZE);
#endif

	if (strcmp(pllswitch, "disable_peri_pll") == 0)
		switch_peri_pll(MHZ(1536));
	else if (((g_binfo.board_id == J2_MM_BOARD_ID) ||
		(g_binfo.board_id == J2_MM_S202_BOARD_ID)) &&
			(strcmp(pllswitch, "") == 0))
		switch_peri_pll(MHZ(1536));
	else
		switch_peri_pll(MHZ(1500));

	spl_dram_init();

#if defined(HB_MMC_BOOT) || defined(HB_NOR_BOOT) \
				|| defined(HB_NAND_BOOT)
	/* write bootinfo to ddr HB_BOOTINFO_ADDR */
	hb_bootinfo_init();
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
#if defined(CONFIG_HB_AP_BOOT) \
	|| defined(HB_MMC_BOOT) \
	|| defined(HB_NOR_BOOT) \
	|| defined(HB_NAND_BOOT)

	return BOOT_DEVICE_RAM;
#elif defined(CONFIG_HB_YMODEM_BOOT)

	return BOOT_DEVICE_UART;
#endif
}

#ifdef CONFIG_SPL_OS_BOOT

#ifdef CONFIG_HB_AP_BOOT
static __maybe_unused void switch_to_el1(void)
{
	armv8_switch_to_el1((u64) SPL_LOAD_DTB_ADDR, 0, 0, 0,
			    SPL_LOAD_OS_ADDR, ES_TO_AARCH64);
}

#endif /* CONFIG_HB_AP_BOOT */

void spl_perform_fixups(struct spl_image_info *spl_image)
{
#if CONFIG_IS_ENABLED(LOAD_FIT)
	unsigned int fdt_size;

	if (spl_image->os == IH_OS_LINUX) {
		fdt_size = fdt_totalsize(spl_image->fdt_addr);

		spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;
		memcpy(spl_image->arg, spl_image->fdt_addr, fdt_size);
	}
#elif defined(CONFIG_HB_AP_BOOT)
	if (ap_boot_type == 1) {
		spl_image->name = "Linux";
		spl_image->os = IH_OS_LINUX;
		spl_image->entry_point = (uintptr_t)switch_to_el1;
		spl_image->size = 0x800000;
		spl_image->arg = (void *)SPL_LOAD_DTB_ADDR;
	}
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

#if defined(HB_SWINFO_BOOT_OFFSET)
int hb_swinfo_boot_spl_check(void)
{
	uint32_t s_magic, s_boot;
	void *s_addr;

	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC)
		s_addr = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_BOOT_OFFSET);
	else
		s_addr = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_BOOT_OFFSET);
	s_boot = readl(s_addr) & 0xF;
	if (s_boot == HB_SWINFO_BOOT_SPLONCE ||
		s_boot == HB_SWINFO_BOOT_SPLWAIT) {
		puts("hang for swinfo boot: ");
		if (s_boot == HB_SWINFO_BOOT_SPLONCE) {
			puts("splonce\n");
			writel(s_boot & (~0xF), s_addr);
		} else {
			puts("splwait\n");
		}
		hang();
	}

	return 0;
}
#endif
