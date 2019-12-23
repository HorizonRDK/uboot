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
#include "serial_hb.h"

#ifndef CONFIG_TARGET_HB_FPGA
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/hb_pinmux.h>
#endif /* CONFIG_TARGET_HB_FPGA */

DECLARE_GLOBAL_DATA_PTR;

/* Set up the baud rate in gd struct */
static void hb_uart_setbrg(struct hb_uart_regs *regs,
	unsigned int clock, unsigned int baud)
{
	unsigned int br_int;
	unsigned long br_frac;
	unsigned int val = 0;

	if (baud <= 0 || clock <= 0)
		return;

	br_int = (clock / (baud * BCR_LOW_MODE));
	br_frac = (clock % (baud * BCR_LOW_MODE));
	br_frac *= 1024;
	br_frac /= (baud * BCR_LOW_MODE);

	val = HB_BCR_MODE(0) | HB_BCR_DIV_INT(br_int) | HB_BCR_DIV_FRAC(br_frac);

	writel(val, &regs->bcr_reg);

	gd->baudrate = baud;
}

/* Initialize the UART, with...some settings. */
static void hb_uart_init(struct hb_uart_regs *regs)
{
	unsigned int val = 0;

	/* Disable uart */
	writel(0x0, &regs->enr_reg);
	
	/* Config uart 8bits 1 stop bit no parity mode */
	val = readl(&regs->lcr_reg);
	val &= (~HB_LCR_STOP) | (~HB_LCR_PEN);
	val |= HB_LCR_BITS;
	writel(val, &regs->lcr_reg);

	/* Clear lsr's error */
	readl(&regs->lsr_reg);

	/* Reset fifos */
	writel(HB_FCR_TFRST | HB_FCR_RFRST, &regs->fcr_reg);

	/* Enable tx and rx and global uart*/
	writel(HB_ENR_EN | HB_ENR_TX_EN | HB_ENR_RX_EN, &regs->enr_reg);
}

static int hb_uart_putc(struct hb_uart_regs *regs, const char c)
{
	if (!(readl(&regs->lsr_reg) & HB_LSR_TX_EMPTY))
		return -EAGAIN;

	writel(c, &regs->tdr_reg);

	return 0;
}

static int hb_uart_getc(struct hb_uart_regs *regs)
{
	unsigned int data;

	/* Wait until there is data in the FIFO */
	if (!(readl(&regs->lsr_reg) & HB_LSR_RXRDY))
		return -EAGAIN;

	data = readl(&regs->rdr_reg);

	return (int)data;
}

static int hb_uart_tstc(struct hb_uart_regs *regs)
{
	return (readl(&regs->lsr_reg) & HB_LSR_RXRDY);
}

#ifndef CONFIG_DM_SERIAL

static struct hb_uart_regs *base_regs __attribute__ ((section(".data")));

static void hb_serial_init_baud(int baudrate)
{
#ifdef CONFIG_TARGET_HB_FPGA
	unsigned int clock = HB_OSC_CLK;
#else
	unsigned int reg = readl(HB_PERISYS_CLK_DIV_SEL);
	unsigned int mdiv = GET_UART_MCLK_DIV(reg);
	unsigned int clock = hb_get_peripll_clk();

	clock = clock / mdiv;
#endif /* CONFIG_TARGET_HB_FPGA */

	base_regs = (struct hb_uart_regs *)CONFIG_DEBUG_UART_BASE;

	hb_uart_setbrg(base_regs, clock, baudrate);
}

int hb_serial_init(void)
{
	struct hb_uart_regs *regs = (struct hb_uart_regs *)CONFIG_DEBUG_UART_BASE;
#ifdef CONFIG_TARGET_HB_FPGA
	unsigned int rate = UART_BAUDRATE_115200;
#else
	unsigned int br_sel = hb_pin_get_uart_br();
	unsigned int rate = (br_sel > 0 ? UART_BAUDRATE_115200 : UART_BAUDRATE_921600);
#endif /* CONFIG_TARGET_HB_FPGA */

	hb_uart_init(regs);
	hb_serial_init_baud(rate);

	return 0;
}

static void hb_serial_putc(const char c)
{
	if (c == '\n')
		while (hb_uart_putc(base_regs, '\r') == -EAGAIN);

	while (hb_uart_putc(base_regs, c) == -EAGAIN);
}

static int hb_serial_getc(void)
{
	while (1) {
		int ch = hb_uart_getc(base_regs);

		if (ch == -EAGAIN) {
			continue;
		}

		return ch;
	}
}

static int hb_serial_tstc(void)
{
	return hb_uart_tstc(base_regs);
}

static void hb_serial_setbrg(void)
{
	hb_serial_init_baud(gd->baudrate);
}

static struct serial_device hb_serial_drv = {
	.name	= "hb_serial",
	.start	= hb_serial_init,
	.stop	= NULL,
	.setbrg	= hb_serial_setbrg,
	.putc	= hb_serial_putc,
	.puts	= default_serial_puts,
	.getc	= hb_serial_getc,
	.tstc	= hb_serial_tstc,
};

void hb_serial_initialize(void)
{
	serial_register(&hb_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &hb_serial_drv;
}

#else

struct hb_uart_priv {
	struct hb_uart_regs *regs;
};

int hb_serial_setbrg(struct udevice *dev, int baudrate)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);
	unsigned int clock;
	unsigned int rate = baudrate;

#if defined(CONFIG_CLK)
	unsigned int br_sel = hb_pin_get_uart_br();
	struct clk mclk;
	int ret;

	rate = br_sel > 0 ? UART_BAUDRATE_115200 : UART_BAUDRATE_921600;

	ret = clk_get_by_index(dev, 0, &mclk);
	if (ret < 0) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clock = clk_get_rate(&mclk);

#else
	clock = CONFIG_DEBUG_UART_CLOCK;
#endif /* CONFIG_CLK */

	hb_uart_setbrg(priv->regs, clock, rate);

	return 0;
}

static int hb_serial_probe(struct udevice *dev)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);

	hb_uart_init(priv->regs);

	return 0;
}

static int hb_serial_getc(struct udevice *dev)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);
	
	return hb_uart_getc(priv->regs);
}

static int hb_serial_putc(struct udevice *dev, const char ch)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);

	return hb_uart_putc(priv->regs, ch);
}

static int hb_serial_pending(struct udevice *dev, bool input)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);
	struct hb_uart_regs *regs = priv->regs;

	if (input)
		return hb_uart_tstc(regs);

	return !!(readl(&regs->lsr_reg) & HB_LSR_TXRDY);
}

static int hb_serial_ofdata_to_platdata(struct udevice *dev)
{
	struct hb_uart_priv *priv = dev_get_priv(dev);

	priv->regs = (struct hb_uart_regs *)devfdt_get_addr(dev);

	return 0;
}

static const struct dm_serial_ops hb_serial_ops = {
	.putc = hb_serial_putc,
	.pending = hb_serial_pending,
	.getc = hb_serial_getc,
	.setbrg = hb_serial_setbrg,
};

static const struct udevice_id hb_serial_ids[] = {
	{ .compatible = "hobot,hb-uart" },
	{ }
};

U_BOOT_DRIVER(serial_hb) = {
	.name	= "serial_hb",
	.id	= UCLASS_SERIAL,
	.of_match = hb_serial_ids,
	.ofdata_to_platdata = hb_serial_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct hb_uart_priv),
	.probe = hb_serial_probe,
	.ops	= &hb_serial_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
#endif /* CONFIG_DM_SERIAL */

#ifdef CONFIG_DEBUG_UART_HB
static inline void _debug_uart_init(void)
{
	struct hb_uart_regs *regs = (struct hb_uart_regs *)CONFIG_DEBUG_UART_BASE;

	hb_uart_init(regs);
	hb_uart_setbrg(regs, CONFIG_DEBUG_UART_CLOCK, CONFIG_BAUDRATE);
}

static inline void _debug_uart_putc(int ch)
{
	struct hb_uart_regs *regs = (struct hb_uart_regs *)CONFIG_DEBUG_UART_BASE;

	while (hb_uart_putc(regs, ch) == -EAGAIN);
}

DEBUG_UART_FUNCS
#endif /* CONFIG_DEBUG_UART_HB */

