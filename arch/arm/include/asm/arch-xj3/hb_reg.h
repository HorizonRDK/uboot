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

#define GPIO_GRP0_REG           0x000
#define GPIO_GRP1_REG           0x010
#define GPIO_GRP2_REG           0x020
#define GPIO_GRP3_REG           0x030
#define GPIO_GRP4_REG           0x040
#define GPIO_GRP5_REG           0x050
#define GPIO_GRP6_REG           0X060
#define GPIO_GRP7_REG           0x070

#define X2_STRAP_PIN_REG                0x140

#define X3_GPIO0_CTRL_REG	0x8
#define X3_GPIO0_VALUE_REG	0xC
#define X3_GPIO1_CTRL_REG	0x18
#define X3_GPIO1_VALUE_REG	0x1c
#define X3_GPIO6_CTRL_REG	0x68
#define X3_GPIO6_VALUE_REG	0x6C

#define I2C_PF5024_SLAVE_ADDR	0x8

#define PIN_BASE_BOARD_SEL(x)	((((x >> 14) & 0x1) << 0x1) | \
	((x >> 12) & 0x1))
#define X3_MIPI_RESET_OUT_LOW(x)	(((x) | 0x10000000) & 0xffffefff)

#define X2_SYSCNT_BASE          (0xA6001000)
#define X2_GPIO_BASE            (0xA6003000)
#define HB_GPIO_MODE            0
#define X2A_PIN_SW_BASE		(0xA6004000)

#define SPACC_BASE	(0xB2100000)
#define PKA_BASE	(0xA1018000)

#endif /* __ASM_ARCH_HB_REG_H__ */
