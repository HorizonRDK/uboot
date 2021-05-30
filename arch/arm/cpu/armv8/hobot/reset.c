/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_timer.h>
#include <asm/io.h>
#include <common.h>
#include <watchdog.h>

void reset_cpu(ulong addr)
{
	/* set reaset reason */
	unsigned int value = 0;

	value = readl(HB_PMU_SW_REG_05) & ~(0xf << HB_RESET_BIT_OFFSET);
	value |= HB_UBOOT_RESET << HB_RESET_BIT_OFFSET;
	writel(value, HB_PMU_SW_REG_05);

	/*SPL will jump to warmboot when this value is not 0*/
        writel(0x0, HB_PMU_SW_REG_19);
        writel(SLEEP_TIME, HB_PMU_SLEEP_PERIOD);
        hb_wdt_init_hw();
        hb_wdt_start(RESET_WDT);
        writel(0x0000100, HB_PMU_POWER_CTRL);
        while (1);
}
