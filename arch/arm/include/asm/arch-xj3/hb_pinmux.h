/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#ifndef __ASM_ARCH_HB_FPGA_PINMUX_H__
#define __ASM_ARCH_HB_FPGA_PINMUX_H__

#define STRAP_PIN_REG           (GPIO_BASE + 0x140)
#define PIN_UART_BR_SEL(x)	(((x) >> 7) & 0x1)
#define BIFSPI_CLK_PIN_REG	(PIN_MUX_BASE + 0x70)
#define PIN_CONFIG_GPIO(x)	((x) | 0x3)
#define PIN_TYPE1_PULL_ENABLE_OFFSET   (6)
#define PIN_TYPE1_PULL_ENABLE    (1 << PIN_TYPE1_PULL_ENABLE_OFFSET)
#define PIN_TYPE1_PULL_DISABLE   (~(1 << PIN_TYPE1_PULL_ENABLE_OFFSET))
#define PIN_TYPE1_PULL_TYPE_OFFSET     (7)
#define PIN_TYPE1_PULL_UP        (1 << PIN_TYPE1_PULL_TYPE_OFFSET)
#define PIN_TYPE1_PULL_DOWN      (~(1 << PIN_TYPE1_PULL_TYPE_OFFSET))
#define PIN_TYPE2_PULL_UP_OFFSET (8)
#define PIN_TYPE2_PULL_UP_ENABLE   (1 << PIN_TYPE2_PULL_UP_OFFSET)
#define PIN_TYPE2_PULL_UP_DISABLE  (~(1 << PIN_TYPE2_PULL_UP_OFFSET))
#define PIN_TYPE2_PULL_DOWN_OFFSET   (7)
#define PIN_TYPE2_PULL_DOWN_ENABLE   (1 << PIN_TYPE2_PULL_DOWN_OFFSET)
#define PIN_TYPE2_PULL_DOWN_DISABLE  (~(1 << PIN_TYPE2_PULL_DOWN_OFFSET))

#define SD0_AIN0_VOL_BIT      (0)
#define SD0_AIN1_VOL_BIT      (1)
#define SD0_AIN0_1V8          (1 << SD0_AIN0_VOL_BIT)
#define SD0_AIN1_1V8          (1 << SD0_AIN1_VOL_BIT)
static inline unsigned int hb_pin_get_uart_br(void)
{
        return (PIN_UART_BR_SEL(readl(STRAP_PIN_REG)));
}
#endif /* __ASM_ARCH_HB_FPGA_PINMUX_H__ */
