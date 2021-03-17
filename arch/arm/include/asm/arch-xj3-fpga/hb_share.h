/*
 * Copyright (C) 2018/04/27 Horizon Robotics Co., Ltd.
 *
 * x2_share.h
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef __ASM_ARCH_HB_SHARE_H__
#define __ASM_ARCH_HB_SHARE_H__

#include <asm/arch/hb_reg.h>

#define HB_SHARE_ROOTFSTYPE_ADDR	(BIF_SPI_BASE + 0x05C)
#define HB_SHARE_INITRD_ADDR		(BIF_SPI_BASE + 0x060)
#define HB_SHARE_BOOT_KERNEL_CTRL	(BIF_SPI_BASE + 0x064)
#define HB_SHARE_DTB_ADDR			(BIF_SPI_BASE + 0x068)
#define HB_SHARE_KERNEL_ADDR		(BIF_SPI_BASE + 0x06C)
#define HB_SHARE_DDRT_BOOT_TYPE		(BIF_SPI_BASE + 0x070)
#define HB_SHARE_DDRT_CTRL			(BIF_SPI_BASE + 0x074)
#define HB_SHARE_DDRT_FW_SIZE		(BIF_SPI_BASE + 0x078)

#define DDRT_WR_RDY_BIT			(1 << 0)
#define DDRT_DW_RDY_BIT			(1 << 8)
#define DDRT_MEM_RDY_BIT		(1 << 9)
#define DDRT_UBOOT_RDY_BIT		(1 << 10)

#endif /* __ASM_ARCH_HB_SHARE_H__ */

