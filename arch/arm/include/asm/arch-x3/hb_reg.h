/*
 * (C) Copyright 2017 - 2018 Horizon Robotics, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_HB_REG_H__
#define __ASM_ARCH_HB_REG_H__

#define SYSCTRL_BASE		(0xA1000000)
#define BIF_SPI_BASE		(0xA1006000)

#define PMU_SYS_BASE		(0xA6000000)
#define PMU_SYSCNT_BASE		(0xA6001000)
#define GPIO_BASE		(0xA6003000)
#define PIN_MUX_BASE    	(0xA6004000)
#define HB_QSPI_BASE		(0xB0000000)

#define SPACC_BASE		(0xB2100000)
#define PKA_BASE		(0xA1018000)

#define HB_GPIO_MODE            0
#endif /* __ASM_ARCH_HB_REG_H__ */
