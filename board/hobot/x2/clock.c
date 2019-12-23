
#include <asm/io.h>
#include <common.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/clock.h>
#include <linux/delay.h>


#ifdef CONFIG_SPL_BUILD
void dram_pll_init(ulong pll_val)
{
	unsigned int value;
	unsigned int try_num = 5;

	printf("set ddr's pll to %lu\n", pll_val / 1000000);

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT, HB_DDRPLL_PD_CTRL);

	switch (pll_val) {
		case MHZ(3200):
			/* Set DDR PLL to 1600 */
			value = FBDIV_BITS(200) | REFDIV_BITS(3) |
				POSTDIV1_BITS(1) | POSTDIV2_BITS(1);
			writel(value, HB_DDRPLL_FREQ_CTRL);

			writel(0x1, HB_DDRSYS_CLK_DIV_SEL);
			break;

		case MHZ(2666):
			/* Set DDR PLL to 1333 */
			value = FBDIV_BITS(111) | REFDIV_BITS(1) |
				POSTDIV1_BITS(2) | POSTDIV2_BITS(1);
			writel(value, HB_DDRPLL_FREQ_CTRL);

			writel(0x1, HB_DDRSYS_CLK_DIV_SEL);
			break;

		case MHZ(2133):
			/* Set DDR PLL to 1066 */
			value = FBDIV_BITS(87) | REFDIV_BITS(1) |
				POSTDIV1_BITS(2) | POSTDIV2_BITS(1);
			writel(value, HB_DDRPLL_FREQ_CTRL);

			writel(0x1, HB_DDRSYS_CLK_DIV_SEL);

			break;

		default:
			break;
	}

	value = readl(HB_DDRPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_DDRPLL_PD_CTRL);

	while (!(value = readl(HB_DDRPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(HB_PLLCLK_SEL);
	value |= DDRCLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	writel(0x1, HB_DDRSYS_CLKEN_SET);

	return;
}

void cnn_pll_init(void)
{
	unsigned int value;
	unsigned int try_num = 5;

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		HB_CNNPLL_PD_CTRL);

	value = readl(HB_CNNPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_CNNPLL_PD_CTRL);

	while (!(value = readl(HB_CNNPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(HB_PLLCLK_SEL);
	value |= CNNCLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	writel(0x33, HB_CNNSYS_CLKEN_SET);
}

void vio_pll_init(void)
{
	unsigned int value;
	unsigned int try_num = 5;

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		HB_VIOPLL_PD_CTRL);

	value = readl(HB_VIOPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_VIOPLL_PD_CTRL);

	while (!(value = readl(HB_VIOPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(HB_PLLCLK_SEL);
	value |= VIOCLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	writel(0x1f, HB_VIOSYS_CLKEN_SET);
}

/* Update Peri PLL */
void switch_peri_pll(ulong pll_val)
{
	unsigned int value;
	unsigned int try_num = 5;

	value =readl(HB_PLLCLK_SEL) & (~PERICLK_SEL_BIT);
	writel(value, HB_PLLCLK_SEL);

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		HB_PERIPLL_PD_CTRL);

	switch (pll_val) {
		case MHZ(1536):
			value = FBDIV_BITS(64) | REFDIV_BITS(1) |
				POSTDIV1_BITS(1) | POSTDIV2_BITS(1);
			break;
		case MHZ(1500):
		default:
			value = FBDIV_BITS(250) | REFDIV_BITS(4) |
				POSTDIV1_BITS(1) | POSTDIV2_BITS(1);
			break;
	}

	writel(value, HB_PERIPLL_FREQ_CTRL);

	value = readl(HB_PERIPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_PERIPLL_PD_CTRL);

	while (!(value = readl(HB_PERIPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(HB_PLLCLK_SEL);
	value |= PERICLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	return;
}
#endif /* CONFIG_SPL_BUILD */

