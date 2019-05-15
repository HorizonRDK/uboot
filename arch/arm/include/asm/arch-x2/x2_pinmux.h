#ifndef __ASM_ARCH_X2_PINMUX_H__
#define __ASM_ARCH_X2_PINMUX_H__

#include <asm/arch/x2_reg.h>

#define STRAP_PIN_REG		(GPIO_BASE + 0x140)

/* GPIO STRAP_PIN */
#define STRAP_PIN_MASK	0xFFFF
#define PIN_1STBOOT_SEL(x)		((x) & 0x1)

#define PIN_2NDBOOT_SEL(x)		(((x) >> 1) & 0x3)
#define PIN_2ND_EMMC		0x0
#define PIN_2ND_SF			0x1
#define PIN_2ND_AP			0x2
#define PIN_2ND_UART		0x3

#define PIN_SF_TYPE_SEL(x)		(((x) >> 3) & 0x1)
#define PIN_DEV_MODE_SEL(x)		(((x) >> 4) & 0x1)

#define PIN_FAST_BOOT_SEL(x)	(((x) >> 5) & 0x3)
#define PIN_FB_CPU_1G		0x0
#define PIN_FB_CPU_500M		0x1
#define PIN_FB_CPU_333M		0x2
#define PIN_FB_CPU_24M		0x3

#define PIN_UART_BR_SEL(x)		(((x) >> 7) & 0x1)
#define PIN_SKIP_CSUM_SEL(x)	(((x) >> 8) & 0x1)
#define PIN_EN_MMU_SEL(x)		(((x) >> 9) & 0x1)
#define PIN_SF_RESET_SEL(x)		(((x) >> 10) & 0x1)
#define PIN_DIS_WDT_SEL(x)		(((x) >> 11) & 0x1)
#define PIN_NID_DUMMY_SEL(x)	(((x) >> 12) & 0x1)
#define PIN_NPL_NUM_SEL(x)      (((x) >> 13) & 0x1)

static inline unsigned int x2_pin_get_uart_br(void)
{
	return (PIN_UART_BR_SEL(readl(STRAP_PIN_REG)));
}

#endif /* __ASM_ARCH_X2_PINMUX_H__ */
