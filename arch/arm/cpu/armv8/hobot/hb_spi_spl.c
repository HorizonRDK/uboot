#if defined(HB_NOR_BOOT) || defined(HB_NAND_BOOT)
#include <common.h>
#include <asm/io.h>
#include <asm/arch/hb_reg.h>
#include <asm/arch/hb_pinmux.h>
#include <spi.h>
#include "hb_spi_spl.h"


#define SPI_TXREG(_base_)			((_base_) + 0x0)
#define SPI_RXREG(_base_) 			((_base_) + 0x0)
#define SPI_SCLK(_base_) 		    ((_base_) + 0x4)
#define SPI_CTRL1(_base_)			((_base_) + 0x8)
#define SPI_CTRL2(_base_)			((_base_) + 0xC)
#define SPI_CTRL3(_base_)			((_base_) + 0x10)
#define SPI_CS(_base_)				((_base_) + 0x14)
#define SPI_STATUS1(_base_)			((_base_) + 0x18)
#define SPI_STATUS2(_base_)			((_base_) + 0x1C)
#define SPI_BATCH_CNT_RX(_base_)	((_base_) + 0x20)
#define SPI_BATCH_CNT_TX(_base_)	((_base_) + 0x24)
#define SPI_FIFO_RXTRIG_LVL(_base_)	((_base_) + 0x28)
#define SPI_FIFO_TXTRIG_LVL(_base_)	((_base_) + 0x2C)
#define SPI_DUAL_QUAD_MODE(_base_)	((_base_) + 0x30)
#define SPI_XIP_CFG(_base_)			((_base_) + 0x34)

#define MSB		 					(0x00)
#define CPHA_H						(0x20)
#define CPHA_L						(0x00)
#define CPOL_H						(0x10)
#define CPOL_L						(0x00)
#define MST_MODE					(0x04)

#define TX_ENABLE					(1 << 3)
#define RX_ENABLE					(1 << 7)

#define FIFO_WIDTH32				(0x02)
#define FIFO_WIDTH16				(0x01)
#define FIFO_WIDTH8					(0x00)
#define FIFO_DEPTH					(16)
#define BATCH_MAX_SIZE				(0x10000)

#define SPI_FIFO_DEPTH				(FIFO_DEPTH)
#define SPI_BACKUP_OFFSET			(0x10000)
#define SPI_CS0						(0x01)
#define SPI_MODE0 					(CPOL_L | CPHA_L)
#define SPI_MODE1 					(CPOL_L | CPHA_H)
#define SPI_MODE2 					(CPOL_H | CPHA_L)
#define SPI_MODE3 					(CPOL_H | CPHA_H)

/* Definition of SPI_CTRL3 */
#define BATCH_DISABLE				(1 << 4)
#define FIFO_RESET					(SW_RST_TX | SW_RST_RX)
#define SW_RST_TX					(1 << 1)
#define SW_RST_RX					(1 << 0)

/* STATUS1 */
#define BATCH_TXDONE				(1 << 5)
#define BATCH_RXDONE				(1 << 4)
#define MODF_CLR					(1 << 3)
#define TX_ALMOST_EMPTY				(1 << 1)
#define RX_ALMOST_FULL				(1 << 0)

/* STATUS2 */
#define TXFIFO_FULL					(1 << 5)
#define RXFIFO_EMPTY				(1 << 4)
#define TXFIFO_EMPTY				(1 << 1)
#define RXFIFO_FULL					(1 << 0)

#define QPI_ENABLE					(1 << 1)
#define DPI_ENABLE					(1 << 0)

/* SPI_XIP_CFG */
#define XIP_EN 						(1 << 0)
#define FLASH_HW_CFG				(1 << 1)
#define FLASH_CTNU_MODE				(1 << 4)

#define XIP_FLASH_ADDR_OFFSET		0x100
/* Batch mode or no batch mode */
#define SPI_BATCH_MODE				1
#define SPI_NO_BATCH_MODE			0

/* Wire mode */
#define SPI_N_MODE					1
#define SPI_D_MODE					2
#define SPI_Q_MODE					4

#define SCLK_DIV(div, scaler)   	((2 << (div)) * (scaler + 1))
#define SCLK_VAL(div, scaler)		(((div) << 4) | ((scaler) << 0))

#define ALIGN_4(x)  (((x) + 3) & ~(0x7))
#define spl_get_sys_noc_aclk()  ({             \
                        unsigned int _u32 = 0; \
                        switch(hb_pin_get_fastboot_sel()) {     \
                        case 0:                                 \
                                _u32 = 500000000;               \
                                break;                          \
                        case 1:                                 \
                                _u32 = 250000000;               \
                                break;                          \
                        case 2:                                 \
                                _u32 = 333000000;               \
                                break;                          \
                        case 3:                                 \
                                _u32 = 12000000;                \
                                break;                          \
                        }                                       \
                        _u32; })

static struct spi_slave g_spi_slave;

int spl_spi_claim_bus(struct spi_slave *slave)
{
	uint64_t base = (uint64_t)slave->memory_map;

	writel(slave->cs, SPI_CS(base));
	return 0;
}

int spl_spi_release_bus(struct spi_slave *slave)
{
	uint64_t base = (uint64_t)slave->memory_map;

	writel(0x0, SPI_CS(base));
	return 0;
}

static uint32_t spl_spi_calc_divider(uint32_t mclk, uint32_t sclk)
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

unsigned int spl_spi_set_speed(struct spi_slave *slave, uint hz)
{
	unsigned int div = 0;
	uint64_t base = (uint64_t) slave->memory_map;
	unsigned int mclk = spl_get_sys_noc_aclk();

	div = spl_spi_calc_divider(mclk, hz);
	writel(div, SPI_SCLK(base));

	return mclk;
}

struct spi_slave *spl_spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct spi_slave *pslave = &g_spi_slave;
	uint32_t val = 0;
	uint64_t base;

	pslave->bus = bus;
	pslave->memory_map = (void *)HB_QSPI_BASE;
	pslave->cs = cs;
	pslave->mode = mode;

	base = (uint64_t)pslave->memory_map;
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
	return pslave;
}

static void spl_spi_set_fifo_width(struct spi_slave *slave, unsigned char width)
{
	uint64_t base = (uint64_t) slave->memory_map;
	uint32_t val;

	if (width != FIFO_WIDTH8 &&
		width != FIFO_WIDTH16 &&
		width != FIFO_WIDTH32) {
		return;
	}

	val = (readl(SPI_CTRL1(base)) & ~0x3);
	val |= width;
	writel(val, SPI_CTRL1(base));

	return;
}

static int spl_spi_check_set(uint64_t reg, uint32_t flag, int32_t timeout)
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

static int spl_spi_check_unset(uint64_t reg, uint32_t flag, int32_t timeout)
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

static int spl_spi_write(struct spi_slave *slave, const void *pbuf, uint32_t len)
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
		if (spl_spi_check_set(SPI_STATUS2(base), TXFIFO_EMPTY, time_out) < 0) {
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

static void spl_spi_cfg_dq_mode(struct spi_slave *slave, uint32_t enable)
{
	uint64_t base = (uint64_t)slave->memory_map;
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

static int spl_spi_read_32(struct spi_slave *slave, uint8_t * pbuf, uint32_t len)
{
	int32_t i = 0;
	uint32_t val = 0;
	uint32_t rx_len = 0;
	uint32_t *ptr = (uint32_t *)pbuf;
	uint32_t rx_remain = len;
	uint32_t time_out = 0x8000000;
	int32_t err;
	uint64_t base = (uint64_t) slave->memory_map;

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
		writel(rx_len, SPI_BATCH_CNT_RX(base));

		val |= RX_ENABLE;
		writel(val, SPI_CTRL1(base));

		for (i = 0; i < rx_len / 8; i++) {
			if (spl_spi_check_set(SPI_STATUS1(base), RX_ALMOST_FULL, 0) < 0) {
				err = -1;
				goto SPI_ERROR;
			}

			*ptr++ = readl(SPI_RXREG(base));
			*ptr++ = readl(SPI_RXREG(base));
		}

		if (spl_spi_check_set(SPI_STATUS1(base), BATCH_RXDONE, time_out) < 0) {
			err = -2;
			goto SPI_ERROR;
		}

		rx_remain -= rx_len;
	} while (rx_remain > 0);

	/* Clear rx done flag */
	writel(BATCH_RXDONE, SPI_STATUS1(base));
	val = readl(SPI_CTRL1(base));
	val &= ~RX_ENABLE;
	writel(val, SPI_CTRL1(base));

	val = readl(SPI_CTRL3(base));
	/* Disable batch mode */
	writel(val | BATCH_DISABLE, SPI_CTRL3(base));

	return 0;

SPI_ERROR:
	val = readl(SPI_CTRL1(base));
	writel(val & ~RX_ENABLE, SPI_CTRL1(base));
	writel(BATCH_RXDONE, SPI_STATUS1(base));
	writel(FIFO_RESET, SPI_CTRL1(base));
	writel(0x0, SPI_CTRL1(base));

	printf("qspi read32 rx = %d\n", err);
	return err;
}

static int spl_spi_read_bytes(struct spi_slave *slave, uint8_t *pbuf, uint32_t len)
{
	int32_t i = 0;
	uint32_t val = 0;
	uint8_t *ptr = (uint8_t *)pbuf;
	uint32_t time_out = 0x8000000;
	int32_t err;
	uintptr_t base = (uintptr_t)slave->memory_map;

	val = readl(SPI_CTRL3(base));
	/* Enable batch mode */
	writel(val & ~BATCH_DISABLE, SPI_CTRL3(base));

	val = readl(SPI_CTRL1(base));
	val &= ~(TX_ENABLE | RX_ENABLE);
	writel(val, SPI_CTRL1(base));
	/* Clear rx done flag */
	writel(BATCH_RXDONE, SPI_STATUS1(base));

	writel(len, SPI_BATCH_CNT_RX(base));

	val |= RX_ENABLE;
	writel(val, SPI_CTRL1(base));

	for (i = 0; i < len; i++) {
		if (spl_spi_check_unset(SPI_STATUS2(base), RXFIFO_EMPTY, time_out) < 0) {
			err = -1;
			goto SPI_ERROR;
		}
		ptr[i] = readl(SPI_RXREG(base)) & 0xFF;
	}

	if (spl_spi_check_set(SPI_STATUS2(base), RXFIFO_EMPTY, time_out) < 0) {
		err = -2;
		goto SPI_ERROR;
	}

	if (spl_spi_check_set(SPI_STATUS1(base), BATCH_RXDONE, time_out) < 0) {
		err = -3;
		goto SPI_ERROR;
	}

	/* Clear rx done flag */
	writel(BATCH_RXDONE, SPI_STATUS1(base));
	val = readl(SPI_CTRL1(base));
	val &= ~RX_ENABLE;
	writel(val, SPI_CTRL1(base));

	val = readl(SPI_CTRL3(base));
	/* Enable batch mode */
	writel(val | BATCH_DISABLE, SPI_CTRL3(base));

	return 0;

SPI_ERROR:
	val = readl(SPI_CTRL1(base));
	writel(val & ~RX_ENABLE, SPI_CTRL1(base));
	writel(BATCH_RXDONE, SPI_CTRL1(base));
	writel(FIFO_RESET, SPI_CTRL1(base));
	writel(0x0, SPI_CTRL1(base));

	printf("qspi read bytes err = %d\n", err);
	return err;
}

static int spl_spi_read_data(struct spi_slave *slave, uint8_t *pbuf, uint32_t len)
{
	uint32_t level;
	uintptr_t base = (uintptr_t)slave->memory_map;
	uint32_t small_len;
	uint32_t large_len;
	int ret = 0;

	level = readl(SPI_FIFO_RXTRIG_LVL(base));
	small_len = len % level;
	large_len = len - small_len;

	if (large_len > 0) {
		spl_spi_set_fifo_width(slave, FIFO_WIDTH32);
		ret = spl_spi_read_32(slave, pbuf, large_len);
		spl_spi_set_fifo_width(slave, FIFO_WIDTH8);
		if (ret < 0) {
			return ret;
		}
	}

	if (small_len > 0) {
		spl_spi_set_fifo_width(slave, FIFO_WIDTH8);
		ret = spl_spi_read_bytes(slave, pbuf + large_len, small_len);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

int spl_spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	uint32_t len;
	int ret = -1;

	if (bitlen == 0) {
		return 0;
	}
	len = bitlen / 8;

	if (dout) {
		ret = spl_spi_write(slave, dout, len);
	} else if (din) {
		spl_spi_cfg_dq_mode(slave, 1);
		ret = spl_spi_read_data(slave, din, len);
		spl_spi_cfg_dq_mode(slave, 0);
	}

#if 0
	if (flags & SPI_XFER_CMD) {
		switch (((u8 *) dout)[0]) {
		case CMD_READ_QUAD_OUTPUT_FAST:
		case CMD_QUAD_PAGE_PROGRAM:
		case CMD_READ_DUAL_OUTPUT_FAST:
			spl_spi_cfg_dq_mode(slave, 1);
			break;
		}
	} else {
		spl_spi_cfg_dq_mode(slave, 0);
	}
#endif /* #if 0 */

	return ret;
}
#endif
