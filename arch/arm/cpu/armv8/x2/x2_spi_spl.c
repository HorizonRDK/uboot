#include <common.h>
#include <asm/io.h>
#include <asm/arch/x2_reg.h>
#include "x2_spi_spl.h"

#ifdef CONFIG_X2_NOR_BOOT
#define ALIGN_4(x)  (((x) + 3) & ~(0x7))

static struct spi_slave g_spi_slave;

void spi_release_bus(struct spi_slave *slave)
{
#if 0
	uint64_t base = (uint64_t) slave->memory_map;
	writel(0x0, SPI_CS(base));
#endif
}

void spi_claim_bus(struct spi_slave *slave)
{
#if 0
	uint64_t base = (uint64_t) slave->memory_map;
	writel(slave->cs, SPI_CS(base));
#endif
}

struct spi_slave *spi_setup_slave(uint32_t spi_num, uint32_t sclk)
{
	struct spi_slave *pslave = &g_spi_slave;

	pslave->bus = spi_num;
	pslave->memory_map = (void *)X2_QSPI_BASE;
	pslave->cs = QSPI_DEV_CS0;
#ifdef CONFIG_QSPI_DUAL
	pslave->mode = SPI_RX_DUAL;
#elif defined(CONFIG_QSPI_QUAD)
	pslave->mode = SPI_RX_QUAD;
#else
	pslave->mode = SPI_RX_SLOW;
#endif /* CONFIG_QSPI_DUAL */

	pslave->speed = sclk;

	return pslave;
}

static void spi_set_fifo_width(struct spi_slave *slave, unsigned char width)
{
	uint64_t base = (uint64_t) slave->memory_map;
	uint32_t val;

	if (width != FIFO_WIDTH8 &&
	    width != FIFO_WIDTH16 && width != FIFO_WIDTH32) {
		return;
	}

	val = (readl(SPI_CTRL1(base)) & ~0x3);
	val |= width;
	writel(val, SPI_CTRL1(base));

	return;
}

static uint32_t spi_calc_divider(uint32_t mclk, uint32_t sclk)
{
	uint32_t div = 0;
	uint32_t div_min;
	uint32_t scaler;

	/* The maxmium of prescale is 16, according to spec. */
	div_min = mclk / sclk / 16;
	if (div_min >= (1 << 16)) {
		printf("invalid qspi freq\n");
		return SCLK_VAL(0xF, 0xF);
	}

	while (div_min >= 1) {
		div_min >>= 1;
		div++;
	}

	scaler = ((mclk / sclk) / (2 << div)) - 1;
	return SCLK_VAL(div, scaler);
}

void spi_init(struct spi_slave *spi, uint32_t mclk, uint32_t sclk)
{
	uint32_t val = 0;
	unsigned int div = 0;
	uint64_t base = (uint64_t) spi->memory_map;

	div = spi_calc_divider(mclk, sclk);
	writel(div, SPI_SCLK(base));

	val = ((SPI_MODE0 & 0x30) | MST_MODE | FIFO_WIDTH8 | MSB);
	writel(val, SPI_CTRL1(base));

	writel(FIFO_DEPTH / 2, SPI_FIFO_RXTRIG_LVL(base));
	writel(FIFO_DEPTH / 2, SPI_FIFO_TXTRIG_LVL(base));
	/* Disable interrupt */
	writel(0x0, SPI_CTRL2(base));
	writel(0x0, SPI_CS(base));

	/* Always set SPI to one line as init. */
	writel(0xFC, SPI_DUAL_QUAD_MODE(base));

	/* Software reset SPI FIFO */
	val = MODF_CLR | BATCH_RXDONE | BATCH_TXDONE;
	writel(val, SPI_STATUS1(base));
	writel(FIFO_RESET, SPI_CTRL3(base));
	writel(0x0, SPI_CTRL3(base));
}

static int spi_check_set(uint64_t reg, uint32_t flag, int32_t timeout)
{
	uint32_t val;

	while (!(val = readl(reg) & flag)) {
		if (timeout == 0) {
			continue;
		}

		timeout -= 1;
		if (timeout == 0) {
			return -1;
		}
	}

	return 0;
}

static int spi_check_unset(uint64_t reg, uint32_t flag, int32_t timeout)
{
	while (readl(reg) & flag) {
		if (timeout == 0) {
			continue;
		}

		timeout -= 1;
		if (timeout == 0) {
			return -1;
		}
	}

	return 0;
}

static int spi_write(struct spi_slave *slave, const void *pbuf, uint32_t len)
{
	uint32_t val;
	uint32_t tx_len;
	uint32_t i;
	const uint8_t *ptr = (const uint8_t *)pbuf;
	uint32_t time_out = 0x8000000;
	int32_t err;
	uint64_t base = (uint64_t) slave->memory_map;

	val = readl(SPI_CTRL3(base));
	writel(val & ~BATCH_DISABLE, SPI_CTRL3(base));

	while (len > 0) {
		tx_len = len < FIFO_DEPTH ? len : FIFO_DEPTH;

		val = readl(SPI_CTRL1(base));
		val &= ~(TX_ENABLE | RX_ENABLE);
		writel(val, SPI_CTRL1(base));
		writel(BATCH_TXDONE, SPI_STATUS1(base));

		/* First write fifo depth data to tx reg */
		writel(tx_len, SPI_FIFO_TXTRIG_LVL(base));
		for (i = 0; i < tx_len; i++) {
			writel(ptr[i], SPI_TXREG(base));
		}

		val |= TX_ENABLE;
		writel(val, SPI_CTRL1(base));
		if (spi_check_set(SPI_STATUS2(base), TXFIFO_EMPTY, time_out) <
		    0) {
			err = -1;
			goto SPI_ERROR;
		}

		len -= tx_len;
		ptr += tx_len;

	}

	writel(BATCH_TXDONE, SPI_STATUS1(base));
	val = readl(SPI_CTRL1(base));
	val &= ~TX_ENABLE;
	writel(val, SPI_CTRL1(base));

	return 0;

SPI_ERROR:
	val = readl(SPI_CTRL1(base));
	writel(val & (~TX_ENABLE), SPI_CTRL1(base));
	writel(BATCH_TXDONE, SPI_STATUS1(base));
	writel(FIFO_RESET, SPI_CTRL3(base));
	writel(0x0, SPI_CTRL3(base));

	printf("qspi tx err%d\n", err);

	return err;

}

static void spi_cfg_dq_mode(struct spi_slave *slave, uint32_t enable)
{
	uint64_t base = (uint64_t) slave->memory_map;
	uint32_t val = 0xFC;

	if (enable) {
		switch (slave->mode) {
		case SPI_RX_DUAL:
			val = DPI_ENABLE;
			break;
		case SPI_RX_QUAD:
			val = QPI_ENABLE;
			break;
		default:
			val = 0xfc;
			break;
		}
	}
	writel(val, SPI_DUAL_QUAD_MODE(base));

	return;
}

static int spi_read_32(struct spi_slave *slave, uint8_t * pbuf, uint32_t len)
{
	int32_t i = 0;
	uint32_t val = 0;
	uint32_t rx_len = 0;
	uint32_t *ptr = (uint32_t *) pbuf;
	uint32_t level;
	uint32_t rx_remain = len;
	uint32_t time_out = 0x8000000;
	int32_t err;
	uint64_t base = (uint64_t) slave->memory_map;

	level = readl(SPI_FIFO_RXTRIG_LVL(base));

	val = readl(SPI_CTRL3(base));
	/* Enable batch mode */
	writel(val & ~BATCH_DISABLE, SPI_CTRL3(base));

	do {
		val = readl(SPI_CTRL1(base));
		val &= ~(TX_ENABLE | RX_ENABLE);
		writel(val, SPI_CTRL1(base));
		/* Clear rx done flag */
		writel(BATCH_RXDONE, SPI_STATUS1(base));

		rx_len = rx_remain < 0x8000 ? rx_remain : 0x8000;
		rx_remain -= rx_len;
		writel(rx_len, SPI_BATCH_CNT_RX(base));

		val |= RX_ENABLE;
		writel(val, SPI_CTRL1(base));

		while (rx_len >= level) {
			if (spi_check_set
			    (SPI_STATUS1(base), RX_ALMOST_FULL, time_out) < 0) {
				err = -1;
				goto SPI_ERROR;
			}

			for (i = 0; i < level / 4; i++) {
				ptr[i] = readl(SPI_RXREG(base));
			}

			ptr += level / 4;
			rx_len -= level;
		}

		if (rx_len > 0) {
			rx_len = ALIGN_4(rx_len);
			for (i = 0; i < rx_len / 4; i++) {
				if (spi_check_unset(SPI_STATUS2(base),
						    RXFIFO_EMPTY,
						    time_out) < 0) {
					err = -1;
					goto SPI_ERROR;
				}
				ptr[i] = readl(SPI_RXREG(base));
			}

			ptr += rx_len / 4;
			rx_len -= rx_len;
		}
#if 0
		if (spi_check_set(SPI_STATUS2(base), RXFIFO_EMPTY, time_out) <
		    0) {
			err = -1;
			goto SPI_ERROR;
		}
#endif
	} while (rx_remain > 0);

	if (spi_check_set(SPI_STATUS1(base), BATCH_RXDONE, time_out) < 0) {
		err = -1;
		goto SPI_ERROR;
	}

	/* Clear rx done flag */
	writel(BATCH_RXDONE, SPI_STATUS1(base));
	val = readl(SPI_CTRL1(base));
	val &= ~(RX_ENABLE | TX_ENABLE);
	writel(val, SPI_CTRL1(base));

	return 0;

SPI_ERROR:
	val = readl(SPI_CTRL1(base));
	writel(val & ~RX_ENABLE, SPI_CTRL1(base));
	writel(BATCH_RXDONE, SPI_CTRL1(base));
	writel(FIFO_RESET, SPI_CTRL1(base));
	writel(0x0, SPI_CTRL1(base));

	printf("qspi rx = %d\n", err);
	return err;
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	uint32_t len;
	uint64_t base = (uint64_t) slave->memory_map;
	int ret = -1;

	if (bitlen == 0) {
		return 0;
	}
	len = bitlen / 8;

	if (flags & SPI_XFER_BEGIN)
		writel(slave->cs, SPI_CS(base));

	if (dout) {
		ret = spi_write(slave, dout, len);
	} else if (din) {
		spi_set_fifo_width(slave, FIFO_WIDTH32);
		ret = spi_read_32(slave, din, len);
		spi_set_fifo_width(slave, FIFO_WIDTH8);
	}

	if (flags & SPI_XFER_END) {
		writel(0x0, SPI_CS(base));
	}

	if (flags & SPI_XFER_CMD) {
		switch (((u8 *) dout)[0]) {
		case CMD_READ_QUAD_OUTPUT_FAST:
		case CMD_QUAD_PAGE_PROGRAM:
		case CMD_READ_DUAL_OUTPUT_FAST:
			spi_cfg_dq_mode(slave, 1);
			break;
		}
	} else {
		spi_cfg_dq_mode(slave, 0);
	}

	return ret;
}
#endif
