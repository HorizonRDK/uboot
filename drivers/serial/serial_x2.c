#include <clk.h>
#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <watchdog.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include <serial.h>
//#include <asm/arch/clk.h>
#include <asm/arch/hardware.h>
#include "serial_x2.h"

#ifdef CONFIG_TARGET_X2
#include <asm/arch/x2_sysctrl.h>
#include <asm/arch/x2_pinmux.h>
#endif /* CONFIG_TARGET_X2 */

DECLARE_GLOBAL_DATA_PTR;

/* Set up the baud rate in gd struct */
static void x2_uart_setbrg(struct x2_uart_regs *regs,
	unsigned int clock, unsigned int baud)
{
	unsigned int br_int;
	unsigned int br_frac;
	unsigned int val = 0;

	if (baud <= 0 || clock <= 0)
		return;

	br_int = (clock / (baud * BCR_LOW_MODE));
	br_frac = (clock % (baud * BCR_LOW_MODE)) * 1024 / (baud * BCR_LOW_MODE);

	val = X2_BCR_MODE(0) | X2_BCR_DIV_INT(br_int) | X2_BCR_DIV_FRAC(br_frac);

	writel(val, &regs->bcr_reg);
}

/* Initialize the UART, with...some settings. */
static void x2_uart_init(struct x2_uart_regs *regs)
{
	unsigned int val = 0;

	/* Disable uart */
	writel(0x0, &regs->enr_reg);
	
	/* Config uart 8bits 1 stop bit no parity mode */
	val = readl(&regs->lcr_reg);
	val &= (~X2_LCR_STOP) | (~X2_LCR_PEN);
	val |= X2_LCR_BITS;
	writel(val, &regs->lcr_reg);

	/* Clear lsr's error */
	readl(&regs->lsr_reg);

	/* Reset fifos */
	writel(X2_FCR_TFRST | X2_FCR_RFRST, &regs->fcr_reg);

	/* Enable tx and rx and global uart*/
	writel(X2_ENR_EN | X2_ENR_TX_EN | X2_ENR_RX_EN, &regs->enr_reg);
}

static int x2_uart_putc(struct x2_uart_regs *regs, const char c)
{
	if (!(readl(&regs->lsr_reg) & X2_LSR_TXRDY))
		return -EAGAIN;

	writel(c, &regs->tdr_reg);

	return 0;
}

static int x2_uart_getc(struct x2_uart_regs *regs)
{
	unsigned int data;

	/* Wait until there is data in the FIFO */
	if (!(readl(&regs->lsr_reg) & X2_LSR_RXRDY))
		return -EAGAIN;

	data = readl(&regs->rdr_reg);

	return (int)data;
}

static int x2_uart_tstc(struct x2_uart_regs *regs)
{
	return (readl(&regs->lsr_reg) & X2_LSR_RXRDY);
}

#ifndef CONFIG_DM_SERIAL

static struct x2_uart_regs *base_regs __attribute__ ((section(".data")));

static void x2_serial_init_baud(int baudrate)
{
#ifdef CONFIG_TARGET_X2_FPGA
	unsigned int clock = X2_OSC_CLK;
#else
	unsigned int reg = readl(X2_PERISYS_CLK_DIV_SEL);
	unsigned int mdiv = GET_UART_MCLK_DIV(reg);
	unsigned int clock = X2_PLL_PERF_CLK / mdiv;
#endif /* CONFIG_TARGET_X2_FPGA */

	base_regs = (struct x2_uart_regs *)CONFIG_DEBUG_UART_BASE;

	x2_uart_setbrg(base_regs, clock, baudrate);
}

int x2_serial_init(void)
{
	struct x2_uart_regs *regs = (struct x2_uart_regs *)CONFIG_DEBUG_UART_BASE;
#ifdef CONFIG_TARGET_X2_FPGA
	unsigned int rate = UART_BAUDRATE_115200;
#else
	unsigned int br_sel = x2_pin_get_uart_br();
	unsigned int rate = (br_sel > 0 ? UART_BAUDRATE_115200 : UART_BAUDRATE_921600);
#endif /* CONFIG_TARGET_X2_FPGA */

	x2_uart_init(regs);
	x2_serial_init_baud(rate);

	return 0;
}

static void x2_serial_putc(const char c)
{
	if (c == '\n')
		while (x2_uart_putc(base_regs, '\r') == -EAGAIN);

	while (x2_uart_putc(base_regs, c) == -EAGAIN);
}

static int x2_serial_getc(void)
{
	while (1) {
		int ch = x2_uart_getc(base_regs);

		if (ch == -EAGAIN) {
			continue;
		}

		return ch;
	}
}

static int x2_serial_tstc(void)
{
	return x2_uart_tstc(base_regs);
}

static void x2_serial_setbrg(void)
{
	x2_serial_init_baud(gd->baudrate);
}

static struct serial_device x2_serial_drv = {
	.name	= "x2_serial",
	.start	= x2_serial_init,
	.stop	= NULL,
	.setbrg	= x2_serial_setbrg,
	.putc	= x2_serial_putc,
	.puts	= default_serial_puts,
	.getc	= x2_serial_getc,
	.tstc	= x2_serial_tstc,
};

void x2_serial_initialize(void)
{
	serial_register(&x2_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &x2_serial_drv;
}

#else

struct x2_uart_priv {
	struct x2_uart_regs *regs;
};

int x2_serial_setbrg(struct udevice *dev, int baudrate)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);
	unsigned long clock;

#if defined(CONFIG_CLK) || defined(CONFIG_SPL_CLK)
	int ret;
	struct clk clk;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clock = clk_get_rate(&clk);
	if (IS_ERR_VALUE(clock)) {
		dev_err(dev, "failed to get rate\n");
		return clock;
	}
	debug("%s: CLK %ld\n", __func__, clock);

	ret = clk_enable(&clk);
	if (ret && ret != -ENOSYS) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}
#else
	//clock = get_uart_clk(0);
	clock = CONFIG_DEBUG_UART_CLOCK;
#endif
	x2_uart_setbrg(priv->regs, clock, baudrate);

	return 0;
}

static int x2_serial_probe(struct udevice *dev)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);

	x2_uart_init(priv->regs);

	return 0;
}

static int x2_serial_getc(struct udevice *dev)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);
	
	return x2_uart_getc(priv->regs);
}

static int x2_serial_putc(struct udevice *dev, const char ch)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);

	return x2_uart_putc(priv->regs, ch);
}

static int x2_serial_pending(struct udevice *dev, bool input)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);
	struct x2_uart_regs *regs = priv->regs;

	if (input)
		return x2_uart_tstc(regs);

	return !!(readl(&regs->lsr_reg) & X2_LSR_TXRDY);
}

static int x2_serial_ofdata_to_platdata(struct udevice *dev)
{
	struct x2_uart_priv *priv = dev_get_priv(dev);

	priv->regs = (struct x2_uart_regs *)devfdt_get_addr(dev);

	return 0;
}

static const struct dm_serial_ops x2_serial_ops = {
	.putc = x2_serial_putc,
	.pending = x2_serial_pending,
	.getc = x2_serial_getc,
	.setbrg = x2_serial_setbrg,
};

static const struct udevice_id x2_serial_ids[] = {
	{ .compatible = "hobot,x2-uart" },
	{ }
};

U_BOOT_DRIVER(serial_x2) = {
	.name	= "serial_x2",
	.id	= UCLASS_SERIAL,
	.of_match = x2_serial_ids,
	.ofdata_to_platdata = x2_serial_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct x2_uart_priv),
	.probe = x2_serial_probe,
	.ops	= &x2_serial_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
#endif /* CONFIG_DM_SERIAL */

#ifdef CONFIG_DEBUG_UART_X2
static inline void _debug_uart_init(void)
{
	struct x2_uart_regs *regs = (struct x2_uart_regs *)CONFIG_DEBUG_UART_BASE;

	x2_uart_init(regs);
	x2_uart_setbrg(regs, CONFIG_DEBUG_UART_CLOCK, CONFIG_BAUDRATE);
}

static inline void _debug_uart_putc(int ch)
{
	struct x2_uart_regs *regs = (struct x2_uart_regs *)CONFIG_DEBUG_UART_BASE;

	while (x2_uart_putc(regs, ch) == -EAGAIN);
}

DEBUG_UART_FUNCS
#endif /* CONFIG_DEBUG_UART_X2 */

