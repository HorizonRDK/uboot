#ifndef __X2_MMC_SPL_H__
#define __X2_MMC_SPL_H__

#include <common.h>

#ifdef CONFIG_X2_MMC_BOOT

#define EMMC_DESC_BASE			0x1000000
#define EMMC_DESC_SIZE			4096	// 0x1000
#define EMMC_SDIO0_MCLK			50000000
#define	EMMC_SDIO0_SCLK			1000000

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

void emmc_init(dw_mmc_params_t * params);
unsigned int emmc_read_blks(int lba, uint64_t buf, unsigned int size);

#endif /* CONFIG_X2_MMC_SPL */

#endif /* __X2_MMC_SPL_H__ */
