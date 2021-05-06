#ifndef __HB_MMC_SPL_H__
#define __HB_MMC_SPL_H__

#include <common.h>

#ifdef HB_MMC_BOOT

#define EMMC_DESC_BASE			0x1000000
#define EMMC_DESC_SIZE			4096	// 0x1000

#define EMMC_BUS_WIDTH_1			0
#define EMMC_BUS_WIDTH_4			1
#define EMMC_BUS_WIDTH_8			2

typedef struct dw_mmc_params {
	unsigned int reg_base;
	unsigned int desc_base;
	unsigned int desc_size;
	int clk_rate;
	int sclk;
	int bus_width;
	unsigned int flags;
} dw_mmc_params_t;

void hb_bootinfo_init(void);

void spl_emmc_init(unsigned int emmc_config);

#endif /* CONFIG_HB_MMC_SPL */

#endif /* __HB_MMC_SPL_H__ */
