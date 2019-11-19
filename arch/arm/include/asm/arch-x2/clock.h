#ifndef __ASM_ARCH_X2_CLOCK_H__
#define __ASM_ARCH_X2_CLOCK_H__

#define MHZ(x)	((x) * 1000000UL)

#ifdef CONFIG_SPL_BUILD
void dram_pll_init(ulong pll_val);

void cnn_pll_init(void);

void vio_pll_init(void);

void switch_peri_pll(ulong pll_val);

#endif /* CONFIG_SPL_BUILD */

#endif /* __ASM_ARCH_X2_CLOCK_H__ */

