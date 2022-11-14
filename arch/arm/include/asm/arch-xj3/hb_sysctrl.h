/*
* Copyright x2 uboot
*/
#ifndef __HB_SYSCTRL_H__
#define __HB_SYSCTRL_H__

#include <asm/arch/hardware.h>
#include <asm/arch/hb_reg.h>

#define HB_SYSPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x010)
#define HB_SYSPLL_PD_CTRL			(SYSCTRL_BASE + 0x014)
#define HB_SYSPLL_STATUS			(SYSCTRL_BASE + 0x018)

#define HB_CNNPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x020)
#define HB_CNNPLL_PD_CTRL			(SYSCTRL_BASE + 0x024)
#define HB_CNNPLL_STATUS			(SYSCTRL_BASE + 0x028)

#define HB_DDRPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x030)
#define HB_DDRPLL_PD_CTRL			(SYSCTRL_BASE + 0x034)
#define HB_DDRPLL_STATUS			(SYSCTRL_BASE + 0x038)

#define HB_VIOPLL_FREQ_CTRL			(SYSCTRL_BASE + 0x040)
#define HB_VIOPLL_PD_CTRL			(SYSCTRL_BASE + 0x044)
#define HB_VIOPLL_STATUS			(SYSCTRL_BASE + 0x048)

#define HB_PERIPLL_FREQ_CTRL		(SYSCTRL_BASE + 0x050)
#define HB_PERIPLL_PD_CTRL			(SYSCTRL_BASE + 0x054)
#define HB_PERIPLL_STATUS			(SYSCTRL_BASE + 0x058)

#define HB_CNNSYS_CLKEN_SET			(SYSCTRL_BASE + 0x124)
#define HB_CNNSYS_CLKEN_CLR			(SYSCTRL_BASE + 0x128)

#define HB_DDRSYS_CLKEN_SET			(SYSCTRL_BASE + 0x134)
#define HB_DDRSYS_CLKEN_CLR			(SYSCTRL_BASE + 0x138)

#define HB_VIOSYS_CLKEN_SET			(SYSCTRL_BASE + 0x144)

#define HB_VIOPLL2_FREQ_CTRL                   (SYSCTRL_BASE + 0x0B0)
#define HB_VIOPLL2_PD_CTRL                     (SYSCTRL_BASE + 0x0B4)
#define HB_VIOPLL2_STATUS                      (SYSCTRL_BASE + 0x0B8)

#define SYS_PCLK_DIV_SEL(x)             (((x) & 0x7) << 12)
#define HB_SYS_PCLK			(SYSCTRL_BASE + 0x204)

#define HB_CNNSYS_CLKOFF_STA		(SYSCTRL_BASE + 0x228)

#define HB_DDRSYS_CLK_DIV_SEL		(SYSCTRL_BASE + 0x230)
#define HB_DDRSYS_CLK_STA			(SYSCTRL_BASE + 0x238)
#define HB_VIOSYS_CLK_DIV_SEL1   	(SYSCTRL_BASE + 0x240)
#define HB_VIOSYS_CLK_DIV_SEL2      (SYSCTRL_BASE + 0x244)
#define HB_VIOSYS_CLK_DIV_SEL3      (SYSCTRL_BASE + 0x24C)
#define HB_PERISYS_CLK_DIV_SEL		(SYSCTRL_BASE + 0x250)

#define HB_PLLCLK_SEL				(SYSCTRL_BASE + 0x300)

#define HB_SD0_CCLK_CTRL			(SYSCTRL_BASE + 0x320)

#define HB_ETH0_CLK_CTRL                (SYSCTRL_BASE + 0x380)
#define HB_ETH0_MODE_CTRL               (SYSCTRL_BASE + 0x384)

#define HB_SYSC_CNNSYS_SW_RSTEN		(SYSCTRL_BASE + 0x420)
#define HB_SYSC_DDRSYS_SW_RSTEN		(SYSCTRL_BASE + 0x430)
#define HB_SYSC_PERISYS_SW_RSTEN	(SYSCTRL_BASE + 0x450)

#define HB_CPU_FLAG			(PMU_SYS_BASE + 0x214)

#define MASK(n)         ((1 << (n)) - 1)
#define REFCLK_DIV(n)   ((n) - 1)

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
#define VIOCLK2_SEL_BIT		(1 << 17)
#define PERICLK_SEL_BIT		(1 << 20)
#define SYSCLK_SEL_BIT		(1 << 24)

/* SYSCTRL VIOSYS_CLK_DIV_SEL1 */
#define IAR_PIX_CLK_SRC_SEL(x)          (((x) & MASK(1)) << 31)
#define SENSOR3_MCLK_2ND_DIV_SEL(x)     (((x) & MASK(3)) << 28)
#define SENSOR2_MCLK_2ND_DIV_SEL(x)     (((x) & MASK(3)) << 24)
#define IAR_PIX_CLK_2ND_DIV_SEL(x)      (((x) & MASK(3)) << 21)
#define IAR_PIX_CLK_1ST_DIV_SEL(x)      (((x) & MASK(5)) << 16)
#define SIF_MCLK_DIV_SEL(x)             (((x) & MASK(4)) << 12)
#define SENSOR_DIV_CLK_SRC_SEL(x)       (((x) & MASK(1)) << 11)
#define SENSOR1_MCLK_2ND_DIV_SEL(x)     (((x) & MASK(3)) << 8)
#define SENSOR0_MCLK_2ND_DIV_SEL(x)     (((x) & MASK(3)) << 5)
#define SENSOR_MCLK_1ST_DIV_SEL(x)      (((x) & MASK(5)) << 0)

/* SYSCTRL VIOSYS_CLK_DIV_SEL2 */
#define MIPI_TX_IPI_CLK_2ND_DIV_SEL(x)  (((x) & MASK(3)) << 29)
#define MIPI_TX_IPI_CLK_1ST_DIV_SEL(x)  (((x) & MASK(5)) << 24)
#define MIPI_CFG_CLK_2ND_DIV_SEL(x)     (((x) & MASK(4)) << 20)
#define MIPI_CFG_CLK_1ST_DIV_SEL(x)     (((x) & MASK(5)) << 15)
#define PYM_MCLK_SRC_SEL(x)             (((x) & MASK(2)) << 12)
#define PYM_MCLK_DIV_SEL(x)             (((x) & MASK(4)) << 8)
#define MIPI_PHY_REFCLK_2ND_DIV_SEL(x)  (((x) & MASK(3)) << 5)
#define MIPI_PHY_REFCLK_1ST_DIV_SEL(x)  (((x) & MASK(5)) << 0)

/* SYSCTRL VIOSYS_CLK_DIV_SEL3 */
#define MIPI_RX3_IPI_CLK_2ND_DIV_SEL(x) (((x) & MASK(3)) << 29)
#define MIPI_RX3_IPI_CLK_1ST_DIV_SEL(x) (((x) & MASK(5)) << 24)
#define MIPI_RX2_IPI_CLK_2ND_DIV_SEL(x) (((x) & MASK(3)) << 21)
#define MIPI_RX2_IPI_CLK_1ST_DIV_SEL(x) (((x) & MASK(5)) << 16)
#define MIPI_RX1_IPI_CLK_2ND_DIV_SEL(x) (((x) & MASK(3)) << 13)
#define MIPI_RX1_IPI_CLK_1ST_DIV_SEL(x) (((x) & MASK(5)) << 8)
#define MIPI_RX0_IPI_CLK_2ND_DIV_SEL(x) (((x) & MASK(3)) << 5)
#define MIPI_RX0_IPI_CLK_1ST_DIV_SEL(x) (((x) & MASK(5)) << 0)

static inline unsigned int hb_get_peripll_clk(void)
{
	unsigned int val = readl(HB_PERIPLL_FREQ_CTRL);
	unsigned int fbdiv, refdiv, postdiv1, postdiv2;

	fbdiv = GET_FBDIV(val);
	refdiv = GET_REFDIV(val);
	postdiv1 = GET_POSTDIV1(val);
	postdiv2 = GET_POSTDIV2(val);

	return ((HB_OSC_CLK / refdiv) *fbdiv / postdiv1 / postdiv2);
}

#endif /* __HB_SYSCTRL_H__ */
