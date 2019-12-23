
#include <asm/io.h>

#include <asm/arch/x2_timer.h>

void x2_timer_init(unsigned int value)
{
	unsigned int reg;

	/* Config sub timer1's mode to one-time */
	reg = readl(TIMER_MODE_REG);
	reg &= ~0xF0;
	writel(reg, TIMER_MODE_REG);

	writel(TM_SUB1_TARGET(value), TIMER_SUB1_TARGET_REG);

	return;
}

void x2_timer_enalbe(void)
{
	writel(TM_SUB1_START(), TIMER_START_REG);

	return;
}

void x2_timer_disable(void)
{
	writel(TM_SUB1_STOP(), TIMER_STOP_REG);

	return;
}

unsigned int x2_timer_get_val32(void)
{
	return readl(TIMER_SUB1_CUR_REG);
}
