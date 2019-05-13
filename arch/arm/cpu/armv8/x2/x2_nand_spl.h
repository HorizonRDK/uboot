#ifndef __X2_NAND_SPL_H__
#define __X2_NAND_SPL_H__

#ifdef CONFIG_X2_NAND_BOOT

#define X2_NAND_MCLK        (500000000)
#define X2_NAND_SCLK        (12500000)
#define CONFIG_QSPI_NAND_QUAD

void spl_nand_init(void);

#endif
#endif
