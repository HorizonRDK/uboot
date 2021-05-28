/*
 * HB watchdog controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * Copyright 2020 Horizon Robotics, Inc.
 */
#include <common.h>
#include <asm/arch/hb_timer.h>
#include <asm/io.h>
#include <watchdog.h>

bool wdt_enabled = false;

void hb_wdt_init_hw(void)
{
	u32 val;
	/* set timer2 to watchdog mode */
	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMRMODE_REG);
	val &= 0xFFFFF0FF;
	val |= (HB_TIMER_WDT_MODE << HB_TIMER_T2MODE_OFFSET);
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRMODE_REG);	

	/* reset watch dog */
	writel(HB_TIMER_WDT_RESET, HB_TIMER_REG_BASE + HB_TIMER_WDCLR_REG);

	/* stop */
	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMREN_REG);
	val |= HB_TIMER_T2STOP;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRSTOP_REG);

	/* disable wdt interrupt */
	writel(HB_TIMER_WDT_INTMASK, HB_TIMER_REG_BASE + HB_TIMER_TMR_SETMASK_REG);
	wdt_enabled = false;

	return;
}

void hb_wdt_start(enum wdt_type type)
{
	u32 bark_count;
	u32 bite_count;
	u32 val;

	if (wdt_enabled) {
		printf("watch dog is already enabled\n");
		return;
	}
	/* reset watch dog*/
	writel(HB_TIMER_WDT_RESET, HB_TIMER_REG_BASE + HB_TIMER_WDCLR_REG);

	/* Fill the count reg */
	if (type == NORMAL_WDT) {
		bark_count = HB_WDT_NORMAL_BARK_TIMEOUT;
		bite_count = HB_WDT_NORMAL_BITE_TIMEOUT;
	} else if (type == RESET_WDT) {
		bark_count = HB_WDT_RESET_TIMEOUT;
		bite_count = HB_WDT_RESET_TIMEOUT;
	} else {
		printf("error watch type:%d\n", type);
		return;
	}
	writel(bark_count, HB_TIMER_REG_BASE + HB_TIMER_WDTGT_REG);
	writel(bite_count, HB_TIMER_REG_BASE + HB_TIMER_WDWAIT_REG);

	/* enable wdt interrupt */
	val = ~(readl(HB_TIMER_REG_BASE + HB_TIMER_TMR_INTMASK_REG));
	val |= HB_TIMER_WDT_INTMASK;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMR_UNMASK_REG);

	/* Start wdt timer */
	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMREN_REG);
	val |= HB_TIMER_T2START;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRSTART_REG);

	/* Unmask bark irq */
	val = ~(readl(HB_TIMER_REG_BASE + HB_TIMER_TMR_INTMASK_REG));
	val |= HB_TIMER_WDT_INTMASK;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMR_UNMASK_REG);
	wdt_enabled = true;
	return;
}

void hb_wdt_stop(void)
{
	u32 val;

	/* reset watch dog*/
	writel(HB_TIMER_WDT_RESET, HB_TIMER_REG_BASE + HB_TIMER_WDCLR_REG);

	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMREN_REG);
	val |= HB_TIMER_T2STOP;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRSTOP_REG);

	/* disable wdt interrupt */
	writel(HB_TIMER_WDT_INTMASK, HB_TIMER_REG_BASE + HB_TIMER_TMR_SETMASK_REG);
	wdt_enabled = false;
	return;
}
