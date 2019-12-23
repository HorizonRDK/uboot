/*
 * HB watchdog controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <asm/arch/hb_timer.h>
#include <asm/io.h>
#include <watchdog.h>

void hb_wdt_init_hw(void)
{
	u32 val;

	/* set timer2 to watchdog mode */
	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMRMODE_REG);
	val &= 0xFFFFF0FF;
	val |= (HB_TIMER_WDT_MODE << HB_TIMER_T2MODE_OFFSET);
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRMODE_REG);	

	return;
}

void hb_wdt_start(void)
{
	u32 bark_count;
	u32 bite_count;
	u32 val;

	/* Fill the count reg */
	bark_count = (HB_WDT_DEFAULT_TIMEOUT / 2) * (HB_TIMER_REF_CLOCK / 24);
	bite_count = (HB_WDT_DEFAULT_TIMEOUT / 2) * (HB_TIMER_REF_CLOCK / 24);
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

	return;
}

void hb_wdt_stop(void)
{
	u32 val;

	val = readl(HB_TIMER_REG_BASE + HB_TIMER_TMREN_REG);
	val |= HB_TIMER_T2STOP;
	writel(val, HB_TIMER_REG_BASE + HB_TIMER_TMRSTOP_REG);

	/* disable wdt interrupt */
	writel(HB_TIMER_WDT_INTMASK, HB_TIMER_REG_BASE + HB_TIMER_TMR_SETMASK_REG);

	return;
}
