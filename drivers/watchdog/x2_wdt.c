/*
 * X2 watchdog controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <x2_timer.h>
#include <asm/io.h>
#include <watchdog.h>

void x2_wdt_init_hw(void)
{
	u32 val;

	/* set timer2 to watchdog mode */
	val = readl(X2_TIMER_REG_BASE + X2_TIMER_TMRMODE_REG);
	val &= 0xFFFFF0FF;
	val |= (X2_TIMER_WDT_MODE << X2_TIMER_T2MODE_OFFSET);
	writel(val, X2_TIMER_REG_BASE + X2_TIMER_TMRMODE_REG);	

	return;
}

void x2_wdt_start(void)
{
	u32 bark_count;
	u32 bite_count;
	u32 val;

	/* Fill the count reg */
	bark_count = (X2_WDT_DEFAULT_TIMEOUT / 2) * (X2_TIMER_REF_CLOCK / 24);
	bite_count = (X2_WDT_DEFAULT_TIMEOUT / 2) * (X2_TIMER_REF_CLOCK / 24);
	writel(bark_count, X2_TIMER_REG_BASE + X2_TIMER_WDTGT_REG);
	writel(bite_count, X2_TIMER_REG_BASE + X2_TIMER_WDWAIT_REG);

	/* enable wdt interrupt */
	val = ~(readl(X2_TIMER_REG_BASE + X2_TIMER_TMR_INTMASK_REG));
	val |= X2_TIMER_WDT_INTMASK;
	writel(val, X2_TIMER_REG_BASE + X2_TIMER_TMR_UNMASK_REG);

	/* Start wdt timer */
	val = readl(X2_TIMER_REG_BASE + X2_TIMER_TMREN_REG);
	val |= X2_TIMER_T2START;
	writel(val, X2_TIMER_REG_BASE + X2_TIMER_TMRSTART_REG);

	/* Unmask bark irq */
	val = ~(readl(X2_TIMER_REG_BASE + X2_TIMER_TMR_INTMASK_REG));
	val |= X2_TIMER_WDT_INTMASK;
	writel(val, X2_TIMER_REG_BASE + X2_TIMER_TMR_UNMASK_REG);

	return;
}

void x2_wdt_stop(void)
{
	u32 val;

	val = readl(X2_TIMER_REG_BASE + X2_TIMER_TMREN_REG);
	val |= X2_TIMER_T2STOP;
	writel(val, X2_TIMER_REG_BASE + X2_TIMER_TMRSTOP_REG);

	/* disable wdt interrupt */
	writel(X2_TIMER_WDT_INTMASK, X2_TIMER_REG_BASE + X2_TIMER_TMR_SETMASK_REG);

	return;
}
