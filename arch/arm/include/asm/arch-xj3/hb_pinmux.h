#ifndef __ASM_ARCH_HB_FPGA_PINMUX_H__
#define __ASM_ARCH_HB_FPGA_PINMUX_H__

#define STRAP_PIN_REG           (GPIO_BASE + 0x140)
#define PIN_UART_BR_SEL(x)	(((x) >> 7) & 0x1)
#define BIFSPI_CLK_PIN_REG	(PIN_MUX_BASE + 0x70)
#define PIN_CONFIG_GPIO(x)	((x) | 0x3)
static inline unsigned int hb_pin_get_uart_br(void)
{
        return (PIN_UART_BR_SEL(readl(STRAP_PIN_REG)));
}
#endif /* __ASM_ARCH_HB_FPGA_PINMUX_H__ */
