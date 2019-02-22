
#include <asm/io.h>
#include <common.h>
#include <asm/arch/x2_sysctrl.h>
#include <asm/arch/clock.h>
#include <linux/delay.h>


#ifdef CONFIG_SPL_BUILD
void dram_pll_init(ulong pll_val)
{
	unsigned int value;
	unsigned int try_num = 5;

	printf("set ddr's pll to %lu\n", pll_val / 1000000);

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT, X2_DDRPLL_PD_CTRL);

	switch (pll_val) {
		case MHZ(3200):
			/* Set DDR PLL to 1600 */
			value = FBDIV_BITS(200) | REFDIV_BITS(3) |
				POSTDIV1_BITS(1) | POSTDIV2_BITS(1);
			writel(value, X2_DDRPLL_FREQ_CTRL);

			writel(0x1, X2_DDRSYS_CLK_DIV_SEL);

			break;

		case MHZ(2133):
			/* Set DDR PLL to 1066 */
			value = FBDIV_BITS(87) | REFDIV_BITS(1) |
				POSTDIV1_BITS(2) | POSTDIV2_BITS(1);
			writel(value, X2_DDRPLL_FREQ_CTRL);

			writel(0x1, X2_DDRSYS_CLK_DIV_SEL);

			break;

		default:
			break;
	}

	value = readl(X2_DDRPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, X2_DDRPLL_PD_CTRL);

	while (!(value = readl(X2_DDRPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(X2_PLLCLK_SEL);
	value |= DDRCLK_SEL_BIT;
	writel(value, X2_PLLCLK_SEL);

	writel(0x1, X2_DDRSYS_CLKEN_SET);

	return;
}

void cnn_pll_init(void)
{
	unsigned int value;
	unsigned int try_num = 5;

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		X2_CNNPLL_PD_CTRL);

	value = readl(X2_CNNPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, X2_CNNPLL_PD_CTRL);

	while (!(value = readl(X2_CNNPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(X2_PLLCLK_SEL);
	value |= CNNCLK_SEL_BIT;
	writel(value, X2_PLLCLK_SEL);

	writel(0x33, X2_CNNSYS_CLKEN_SET);
}

void vio_pll_init(void)
{
	unsigned int value;
	unsigned int try_num = 5;

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		X2_VIOPLL_PD_CTRL);

	value = readl(X2_VIOPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, X2_VIOPLL_PD_CTRL);

	while (!(value = readl(X2_VIOPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(X2_PLLCLK_SEL);
	value |= VIOCLK_SEL_BIT;
	writel(value, X2_PLLCLK_SEL);

	writel(0x1f, X2_VIOSYS_CLKEN_SET);
}

#endif /* CONFIG_SPL_BUILD */

