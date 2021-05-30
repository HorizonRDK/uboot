/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#ifndef __HB_PMU_H__
#define __HB_PMU_H__

#include <asm/arch/hb_reg.h>

#define HB_PMU_SLEEP_PERIOD		(PMU_SYS_BASE + 0x0000)
#define HB_PMU_SLEEP_CMD		(PMU_SYS_BASE + 0x0004)
#define HB_PMU_WAKEUP_STA		(PMU_SYS_BASE + 0x0008)
#define HB_PMU_OUTPUT_CTRL		(PMU_SYS_BASE + 0x000C)
#define HB_PMU_VDD_CNN_CTRL		(PMU_SYS_BASE + 0x0010)
#define HB_PMU_DDRSYS_CTRL		(PMU_SYS_BASE + 0x0014)
#define HB_PMU_POWER_CTRL		(PMU_SYS_BASE + 0x0020)

#define HB_PMU_W_SRC			(PMU_SYS_BASE + 0x0030)
#define HB_PMU_W_SRC_MASK		(PMU_SYS_BASE + 0x0040)

#define HB_PMU_SW_REG_00		(PMU_SYS_BASE + 0x0200)
#define HB_PMU_SW_REG_01		(PMU_SYS_BASE + 0x0204)
#define HB_PMU_SW_REG_02               (PMU_SYS_BASE + 0x0208)
#define HB_PMU_SW_REG_03               (PMU_SYS_BASE + 0x020c)
#define HB_PMU_SW_REG_04               (PMU_SYS_BASE + 0x0210)
#define HB_PMU_SW_REG_05               (PMU_SYS_BASE + 0x0214)
#define HB_PMU_SW_REG_19               (PMU_SYS_BASE + 0x024c)
#define HB_PMU_SW_REG_23               (PMU_SYS_BASE + 0x025C)
#define HB_PMU_SW_REG_27               (PMU_SYS_BASE + 0x026c)
#define HB_PMU_SW_REG_28               (PMU_SYS_BASE + 0x0270)
#define HB_PMU_SW_REG_29               (PMU_SYS_BASE + 0x0274)
#define HB_PMU_SW_REG_30               (PMU_SYS_BASE + 0x0278)
#define HB_PMU_SW_REG_31               (PMU_SYS_BASE + 0x027c)


#define SLEEP_TIME			0x75300 //20ms when 24M
#define HB_RESET_BIT_OFFSET             28
#define HB_UBOOT_RESET                  3
#endif /* __HB_PMU_H__ */
