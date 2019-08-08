#ifndef __X2_PMU_H__
#define __X2_PMU_H__

#include <asm/arch/x2_reg.h>

#define X2_PMU_SLEEP_PERIOD		(PMU_SYS_BASE + 0x0000)
#define X2_PMU_SLEEP_CMD		(PMU_SYS_BASE + 0x0004)
#define X2_PMU_WAKEUP_STA		(PMU_SYS_BASE + 0x0008)
#define X2_PMU_OUTPUT_CTRL		(PMU_SYS_BASE + 0x000C)
#define X2_PMU_VDD_CNN_CTRL		(PMU_SYS_BASE + 0x0010)
#define X2_PMU_DDRSYS_CTRL		(PMU_SYS_BASE + 0x0014)
#define X2_PMU_POWER_CTRL		(PMU_SYS_BASE + 0x0020)

#define X2_PMU_W_SRC			(PMU_SYS_BASE + 0x0030)
#define X2_PMU_W_SRC_MASK		(PMU_SYS_BASE + 0x0040)

#define X2_PMU_SW_REG_00		(PMU_SYS_BASE + 0x0200)
#define X2_PMU_SW_REG_01		(PMU_SYS_BASE + 0x0204)
#define X2_PMU_SW_REG_02               (PMU_SYS_BASE + 0x0208)
#define X2_PMU_SW_REG_03               (PMU_SYS_BASE + 0x020c)
#define X2_PMU_SW_REG_04               (PMU_SYS_BASE + 0x0210)
#define X2_PMU_SW_REG_27               (PMU_SYS_BASE + 0x026c)
#define X2_PMU_SW_REG_28               (PMU_SYS_BASE + 0x0270)
#define X2_PMU_SW_REG_29               (PMU_SYS_BASE + 0x0274)
#define X2_PMU_SW_REG_30               (PMU_SYS_BASE + 0x0278)
#define X2_PMU_SW_REG_31               (PMU_SYS_BASE + 0x027c)

#endif /* __X2_PMU_H__ */

