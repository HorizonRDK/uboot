#ifndef __ASM_ARCH_X2_SHARE_H__
#define __ASM_ARCH_X2_SHARE_H__

#include <asm/arch/x2_reg.h>

#define X2_SHARE_BOOT_KERNEL_CTRL	(BIF_SPI_BASE + 0x064)
#define X2_SHARE_DTB_ADDR		(BIF_SPI_BASE + 0x068)
#define X2_SHARE_KERNEL_ADDR	(BIF_SPI_BASE + 0x06C)
#define X2_SHARE_DDRT_BOOT_TYPE	(BIF_SPI_BASE + 0x070)
#define X2_SHARE_DDRT_CTRL		(BIF_SPI_BASE + 0x074)
#define X2_SHARE_DDRT_FW_SIZE	(BIF_SPI_BASE + 0x078)

#define DDRT_WR_RDY_BIT			(1 << 0)
#define DDRT_DW_RDY_BIT			(1 << 8)
#define DDRT_MEM_RDY_BIT		(1 << 9)
#define DDRT_UBOOT_RDY_BIT		(1 << 10)

#endif /* __ASM_ARCH_X2_SHARE_H__ */

