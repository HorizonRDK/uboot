#ifndef __X2_SYSCTRL_H__
#define __X2_SYSCTRL_H__

#include <asm/arch/x2_reg.h>

#define X2_DDRPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x030)
#define X2_DDRPLL_PD_CTRL			(SYSCTRL_BASE + 0x034)
#define X2_DDRPLL_STATUS			(SYSCTRL_BASE + 0x038)

#define X2_DDRSYS_CLK_DIV_SEL		(SYSCTRL_BASE + 0x230)
#define X2_DDRSYS_CLK_STA			(SYSCTRL_BASE + 0x238)

#define X2_PLLCLK_SEL				(SYSCTRL_BASE + 0x300)

#define X2_SYSC_DDRSYS_SW_RSTEN		(SYSCTRL_BASE + 0x430)

/* DDRPLL_FREQ_CTRL */
#define FBDIV_BITS(x)		((x & 0xFFF) << 0)
#define REFDIV_BITS(x)		((x & 0x3F) << 12)
#define POSTDIV1_BITS(x)	((x & 0x7) << 20)
#define POSTDIV2_BITS(x)	((x & 0x7) << 24)

/* DDRPLL_PD_CTRL */
#define PD_BIT				(1 << 0)
#define DSMPD_BIT			(1 << 4)
#define FOUTPOST_DIV_BIT	(1 << 8)
#define FOUTVCO_BIT			(1 << 12)
#define BYPASS_BIT			(1 << 16)

/* DDRPLL_STATUS */
#define LOCK_BIT		(1 << 0)

/* SYSCTRL PLL_CLK_SEL */
#define ARMPLL_SEL_BIT		(1 << 0)
#define CPUCLK_SEL_BIT		(1 << 4)
#define CNNCLK_SEL_BIT		(1 << 8)
#define DDRCLK_SEL_BIT		(1 << 12)
#define VIOCLK_SEL_BIT		(1 << 16)
#define PERICLK_SEL_BIT		(1 << 20)


#endif /* __X2_SYSCTRL_H__ */
