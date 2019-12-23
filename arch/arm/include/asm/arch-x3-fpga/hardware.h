/*
 * (C) Copyright 2017 - 2018 Horizon Robotics, Inc.
 * yu.xing <yu.xing@hobot.cc>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_HARDWARE_H__
#define __ASM_ARCH_HARDWARE_H__

#ifdef CONFIG_TARGET_X3_FPGA_HAPS
#define HB_OSC_CLK			(10000000)
#else
#define HB_OSC_CLK			(20000000)
#endif
#endif /* __ASM_ARCH_HARDWARE_H__ */
