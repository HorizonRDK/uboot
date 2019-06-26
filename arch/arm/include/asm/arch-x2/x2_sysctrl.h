#ifndef __X2_SYSCTRL_H__
#define __X2_SYSCTRL_H__

#include <asm/arch/hardware.h>
#include <asm/arch/x2_reg.h>

#define X2_CNNPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x020)
#define X2_CNNPLL_PD_CTRL			(SYSCTRL_BASE + 0x024)
#define X2_CNNPLL_STATUS			(SYSCTRL_BASE + 0x028)

#define X2_DDRPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x030)
#define X2_DDRPLL_PD_CTRL			(SYSCTRL_BASE + 0x034)
#define X2_DDRPLL_STATUS			(SYSCTRL_BASE + 0x038)

#define X2_VIOPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x040)
#define X2_VIOPLL_PD_CTRL			(SYSCTRL_BASE + 0x044)
#define X2_VIOPLL_STATUS			(SYSCTRL_BASE + 0x048)

#define X2_PERIPLL_FREQ_CTRL		(SYSCTRL_BASE + 0x050)
#define X2_PERIPLL_PD_CTRL			(SYSCTRL_BASE + 0x054)
#define X2_PERIPLL_STATUS			(SYSCTRL_BASE + 0x058)

#define X2_CNNSYS_CLKEN_SET			(SYSCTRL_BASE + 0x124)
#define X2_CNNSYS_CLKEN_CLR			(SYSCTRL_BASE + 0x128)

#define X2_DDRSYS_CLKEN_SET			(SYSCTRL_BASE + 0x134)
#define X2_DDRSYS_CLKEN_CLR			(SYSCTRL_BASE + 0x138)

#define X2_VIOSYS_CLKEN_SET			(SYSCTRL_BASE + 0x144)

#define X2_CNNSYS_CLKOFF_STA		(SYSCTRL_BASE + 0x228)

#define X2_DDRSYS_CLK_DIV_SEL		(SYSCTRL_BASE + 0x230)
#define X2_DDRSYS_CLK_STA			(SYSCTRL_BASE + 0x238)

#define X2_PERISYS_CLK_DIV_SEL		(SYSCTRL_BASE + 0x250)

#define X2_PLLCLK_SEL				(SYSCTRL_BASE + 0x300)

#define X2_SD0_CCLK_CTRL			(SYSCTRL_BASE + 0x320)

#define X2_SYSC_CNNSYS_SW_RSTEN		(SYSCTRL_BASE + 0x420)
#define X2_SYSC_DDRSYS_SW_RSTEN		(SYSCTRL_BASE + 0x430)

/* DDRPLL_FREQ_CTRL */
#define FBDIV_BITS(x)		((x & 0xFFF) << 0)
#define REFDIV_BITS(x)		((x & 0x3F) << 12)
#define POSTDIV1_BITS(x)	((x & 0x7) << 20)
#define POSTDIV2_BITS(x)	((x & 0x7) << 24)

#define GET_FBDIV(x)		((x) & 0xFFF)
#define GET_REFDIV(x)		(((x) >> 12) & 0x3F)
#define GET_POSTDIV1(x)		(((x) >> 20) & 0x7)
#define GET_POSTDIV2(x)		(((x) >> 24) & 0x7)

/* DDRPLL_PD_CTRL */
#define PD_BIT				(1 << 0)
#define DSMPD_BIT			(1 << 4)
#define FOUTPOST_DIV_BIT	(1 << 8)
#define FOUTVCO_BIT			(1 << 12)
#define BYPASS_BIT			(1 << 16)

/* DDRPLL_STATUS */
#define LOCK_BIT		(1 << 0)

/* SYSCTRL PERISYS_CLK_DIV_SEL */
#define UART_MCLK_DIV_SEL(x)		(((x) & 0xF) << 4)
#define GET_UART_MCLK_DIV(x)		((((x) >> 4) & 0xF) + 1)

/* SYSCTRL PLL_CLK_SEL */
#define ARMPLL_SEL_BIT		(1 << 0)
#define CPUCLK_SEL_BIT		(1 << 4)
#define CNNCLK_SEL_BIT		(1 << 8)
#define DDRCLK_SEL_BIT		(1 << 12)
#define VIOCLK_SEL_BIT		(1 << 16)
#define PERICLK_SEL_BIT		(1 << 20)

static inline unsigned int x2_get_peripll_clk(void)
{
	unsigned int val = readl(X2_PERIPLL_FREQ_CTRL);
	unsigned int fbdiv, refdiv, postdiv1, postdiv2;

	fbdiv = GET_FBDIV(val);
	refdiv = GET_REFDIV(val);
	postdiv1 = GET_POSTDIV1(val);
	postdiv2 = GET_POSTDIV2(val);

	return ((X2_OSC_CLK / refdiv) *fbdiv / postdiv1 / postdiv2);
}

#endif /* __X2_SYSCTRL_H__ */
