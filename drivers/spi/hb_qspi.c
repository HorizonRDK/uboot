/* SPDX-License-Identifier: GPL-2.0+
 *
 * QSPI driver
 *
 * Copyright (C) 2019, Horizon Robotics, <yang.lu@horizon.ai>
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <ubi_uboot.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <dm/device-internal.h>
#include <clk.h>
#include <linux/kernel.h>
#include <spi-mem.h>
#include "./hb_qspi.h"

DECLARE_GLOBAL_DATA_PTR;

// #define QSPI_DEBUG

/**
 * struct hb_qspi_platdata - platform data for Hobot QSPI
 *
 * @regs: Point to the base address of QSPI registers
 * @hclk: QSPI input hclk frequency
 * @sclk: Target BUS sclk frequency
 * @xfer_mode: 0-Byte 1-batch 2-dma, Default work in byte mode
 * @num_cs: The maximum number of cs
 */
static struct hb_qspi_platdata {
	void __iomem *regs_base;
	u32 hclk;
	u32 sclk;
	u32 xfer_mode;
	u32 num_cs;
	u32 bus_width;
} *plat;

static struct hb_qspi_priv {
	void __iomem *regs_base;
} *hbqspi;

bool speed_set = false;

#define hb_qspi_rd_reg(dev, reg)	   readl((dev)->regs_base + (reg))
#define hb_qspi_wr_reg(dev, reg, val)  writel((val), (dev)->regs_base + (reg))

#ifdef QSPI_DEBUG
void qspi_dump_reg(struct hb_qspi_priv *xqspi)
{
	uint32_t val = 0, i;

	for (i = HB_QSPI_DAT_REG; i <= HB_QSPI_XIP_REG; i = i + 4) {
		val = hb_qspi_rd_reg(xqspi, i);
		printf("reg[0x%08x] ==> [0x%08x]\n", xqspi->regs_base + i, val);
	}
}

void trace_transfer(const void * buf, u32 len, bool dir)
{
#define __TRACE_BUF_SIZE__ 128
	int i = 0;
	const u8 *tmpbuf = NULL; // kzalloc(tmpbufsize, GFP_KERNEL);
	char tmp_prbuf[32];
	char prbuf[__TRACE_BUF_SIZE__] = { 0 };

	tmpbuf = (const u8 *) buf;
	snprintf(prbuf, __TRACE_BUF_SIZE__, "%s-%s[L:%d] ",
		dir ? "<" : "", dir ? "" : ">", len);

	if(len) {
		snprintf(tmp_prbuf, sizeof(tmp_prbuf), " ");
		strcat(prbuf, tmp_prbuf);
#define QSPI_DEBUG_DATA_LEN	16
		for (i = 0; i < ((len < QSPI_DEBUG_DATA_LEN) ?
			len : QSPI_DEBUG_DATA_LEN); i++) {
			snprintf(tmp_prbuf, sizeof(tmp_prbuf), "%02X ", tmpbuf[i]);
			strcat(prbuf, tmp_prbuf);
		}
	}
	printf("%s\n", prbuf);
}
#endif

static inline int set_speed(uint speed)
{
	int confr, prescaler, divisor;
	unsigned int max_br, min_br, br_div;
	if (speed_set)
		return 0;

	max_br = plat->hclk / 2;
	min_br = plat->hclk / 1048576;
	if (speed > max_br) {
		debug("Warning:speed[%d] > max_br[%d],speed will be set to default br: %d\n",
				speed, max_br, HB_QSPI_DEF_BR);
		speed = HB_QSPI_DEF_BR;
	}
	if (speed < min_br) {
		debug("Warning:speed[%d] < min_br[%d],speed will be set to default br: %d\n",
				speed, min_br, HB_QSPI_DEF_BR);
		speed = HB_QSPI_DEF_BR;
	}

	for (prescaler = 15; prescaler >= 0; prescaler--) {
		for (divisor = 15; divisor >= 0; divisor--) {
			br_div = (prescaler + 1) * (2 << divisor);
			if ((plat->hclk / br_div) >= speed) {
				confr = (prescaler | (divisor << 4)) & 0xFF;

				hb_qspi_wr_reg(hbqspi, HB_QSPI_BDR_REG, confr);
				speed_set = true;
				plat->sclk = plat->hclk / br_div;
				return 0;
			}
		}
	}
	return 0;
}

static inline int hb_qspi_set_speed(struct udevice *bus, uint speed)
{
	return set_speed(speed);
}

static inline int hb_qspi_set_mode(struct udevice *bus, uint mode)
{
	unsigned int val = 0;

	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL1_REG);
	if (mode & SPI_CPHA)
		val |= HB_QSPI_CPHA;
	if (mode & SPI_CPOL)
		val |= HB_QSPI_CPOL;
	if (mode & SPI_LSB_FIRST)
		val |= HB_QSPI_LSB;
	if (mode & SPI_SLAVE)
		val &= (~HB_QSPI_MST);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL1_REG, val);

	return 0;
}

/* currend driver only support BYTE/DUAL/QUAD for both RX and TX */
static void hb_qspi_set_wire(struct hb_qspi_priv *hbqspi, uint mode)
{
	uint32_t val = hb_qspi_rd_reg(hbqspi, HB_QSPI_DQM_REG) & 0xFC;

	switch (mode) {
	case 1:
		break;
	case 2:
		val |= HB_QSPI_DUAL;
		break;
	case 4:
		val |= HB_QSPI_QUAD;
		break;
	default:
		break;
	}

	hb_qspi_wr_reg(hbqspi, HB_QSPI_DQM_REG, val);
}

static int hb_qspi_claim_bus(struct udevice *dev)
{
	/* Now Hobot qspi Controller is always on */
	return 0;
}

static int hb_qspi_release_bus(struct udevice *dev)
{
	/* Now Hobot qspi Controller is always on */
	return 0;
}

static inline void hb_qspi_reset_fifo(struct hb_qspi_priv *hbqspi)
{
	u32 val;

	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL3_REG);
	val |= HB_QSPI_RST_ALL;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL3_REG, val);
	val = (~HB_QSPI_RST_ALL);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL3_REG, val);

	return;
}

/* tx almost empty */
static inline int hb_qspi_tx_ae(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST1_REG);
		trys++;
	} while ((!(val & HB_QSPI_TX_AE)) && (trys < TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

/* rx almost full */
static inline int hb_qspi_rx_af(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST1_REG);
		trys++;
	} while ((!(val & HB_QSPI_RX_AF)) && (trys < TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

static inline int hb_qspi_tb_done(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST1_REG);
		trys++;
	} while ((!(val & HB_QSPI_TBD)) && (trys < TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_ST1_REG, (val | HB_QSPI_TBD));


	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

static inline int hb_qspi_rb_done(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST1_REG);
		trys++;
	} while ((!(val & HB_QSPI_RBD)) && (trys < TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_ST1_REG, (val | HB_QSPI_RBD));


	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

static inline int hb_qspi_tx_full(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST2_REG);
		trys++;
	} while ((val & HB_QSPI_TX_FULL) && (trys < TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

static inline int hb_qspi_tx_empty(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST2_REG);
		trys++;
	} while ((!(val & HB_QSPI_TX_EP)) && (trys < TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

static inline int hb_qspi_rx_empty(struct hb_qspi_priv *hbqspi)
{
	u32 val, trys = 0;

	do {
		val = hb_qspi_rd_reg(hbqspi, HB_QSPI_ST2_REG);
		trys++;
	} while ((val & HB_QSPI_RX_EP) && (trys < TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys < TRYS_TOTAL_NUM ? 0 : -1;
}

/* config fifo width */
static inline void hb_qspi_set_fw(struct hb_qspi_priv *hbqspi, u32 fifo_width)
{
	u32 val;

	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL1_REG);
	val &= (~0x3);
	val |= fifo_width;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL1_REG, val);

	return;
}

/*config xfer mode:enable/disable BATCH/RX/TX */
static inline void hb_qspi_set_xfer(struct hb_qspi_priv *hbqspi, u32 op_flag)
{
	u32 ctl1_val = 0, ctl3_val = 0;

	ctl1_val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL1_REG);
	ctl3_val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL3_REG);

	switch (op_flag) {
	case HB_QSPI_OP_RX_EN:
		ctl1_val |= HB_QSPI_RX_EN;
		break;
	case HB_QSPI_OP_RX_DIS:
		ctl1_val &= (~HB_QSPI_RX_EN);
		break;
	case HB_QSPI_OP_TX_EN:
		ctl1_val |= HB_QSPI_TX_EN;
		break;
	case HB_QSPI_OP_TX_DIS:
		ctl1_val &= (~HB_QSPI_TX_EN);
		break;
	case HB_QSPI_OP_BAT_EN:
		ctl3_val &= (~HB_QSPI_BATCH_DIS);
		break;
	case HB_QSPI_OP_BAT_DIS:
		ctl3_val |= HB_QSPI_BATCH_DIS;
		break;
	default:
		printf("Op(0x%x) if error, please check it!\n", op_flag);
		break;
	}
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL1_REG, ctl1_val);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL3_REG, ctl3_val);

	return;
}

static inline int hb_qspi_rd_batch(struct hb_qspi_priv *hbqspi,
							void *pbuf, uint32_t len)
{
	u32 i, rx_len, offset = 0, ret = 0;
	int64_t tmp_len = len;
	u32 *dbuf = (u32 *) pbuf;

	/* Enable batch mode */
	hb_qspi_set_fw(hbqspi, HB_QSPI_FW32);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		rx_len = MIN(tmp_len, BATCH_MAX_CNT);
		hb_qspi_wr_reg(hbqspi, HB_QSPI_RBC_REG, rx_len);
		/* enbale rx */
		hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_RX_EN);

		for (i=0; i< rx_len; i+=8) {
			if (hb_qspi_rx_af(hbqspi)) {
				ret = -1;
				goto rb_err;
			}
			dbuf[offset++] = hb_qspi_rd_reg(hbqspi, HB_QSPI_DAT_REG);
			dbuf[offset++] = hb_qspi_rd_reg(hbqspi, HB_QSPI_DAT_REG);
		}
		if (hb_qspi_rb_done(hbqspi)) {
			printf("%s_%d:rx failed! len=%d, received=%d, i=%d\n",
					__func__, __LINE__, len, offset, i);
			ret = -1;
			goto rb_err;
		}
		hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_RX_DIS);
		tmp_len = tmp_len - rx_len;
	}

rb_err:
	/* Disable batch mode and rx link */
	hb_qspi_set_fw(hbqspi, HB_QSPI_FW8);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_BAT_DIS);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_RX_DIS);

	return ret;
}

static inline int hb_qspi_wr_batch(struct hb_qspi_priv *hbqspi,
							const void *pbuf, uint32_t len)
{
	u32 i, tx_len, offset = 0, tmp_len = len, ret = 0;
	u32 *dbuf = (u32 *) pbuf;

	hb_qspi_set_fw(hbqspi, HB_QSPI_FW32);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		tx_len = MIN(tmp_len, BATCH_MAX_CNT);
		hb_qspi_wr_reg(hbqspi, HB_QSPI_TBC_REG, tx_len);

		/* enbale tx */
		hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_TX_EN);

		for (i=0; i < tx_len; i += 8) {
			if (hb_qspi_tx_ae(hbqspi)) {
				ret = -1;
				goto tb_err;
			}
			hb_qspi_wr_reg(hbqspi, HB_QSPI_DAT_REG, dbuf[offset++]);
			hb_qspi_wr_reg(hbqspi, HB_QSPI_DAT_REG, dbuf[offset++]);
		}
		if (hb_qspi_tb_done(hbqspi)) {
			printf("%s_%d:tx failed! len=%d, received=%d, i=%d\n",
					__func__, __LINE__, len, offset, i);
			ret = -1;
			goto tb_err;
		}
		tmp_len = tmp_len - tx_len;
	}

tb_err:
	/* Disable batch mode and tx link */
	hb_qspi_set_fw(hbqspi, HB_QSPI_FW8);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_BAT_DIS);
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_TX_DIS);

	return ret;
}

static inline int hb_qspi_rd_byte(struct hb_qspi_priv *hbqspi,
							void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale rx */
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_RX_EN);

	for (i = 0; i < len; i++) {
		if (hb_qspi_tx_empty(hbqspi)) {
			ret = -1;
			goto rd_err;
		}
		hb_qspi_wr_reg(hbqspi, HB_QSPI_DAT_REG, 0x00);
		if (hb_qspi_rx_empty(hbqspi)) {
			ret = -1;
			goto rd_err;
		}
		dbuf[i] = hb_qspi_rd_reg(hbqspi, HB_QSPI_DAT_REG) & 0xFF;
	}

rd_err:
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_RX_DIS);

	if (0 != ret)
		printf("%s_%d:read op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static inline int hb_qspi_wr_byte(struct hb_qspi_priv *hbqspi,
							const void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale tx */
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_TX_EN);

	for (i = 0; i < len; i++) {
		if (hb_qspi_tx_full(hbqspi)) {
			ret = -1;
			goto wr_err;
		}
		hb_qspi_wr_reg(hbqspi, HB_QSPI_DAT_REG, dbuf[i]);
	}
	/* Check tx complete */
	if (hb_qspi_tx_empty(hbqspi))
		ret = -1;

wr_err:
	hb_qspi_set_xfer(hbqspi, HB_QSPI_OP_TX_DIS);

	if (0 != ret)
		printf("%s_%d:write op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static inline int hb_qspi_read(struct hb_qspi_priv *hbqspi,
						void *pbuf, uint32_t len)
{
	int32_t ret = 0;
	u32 remainder = len % HB_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = hb_qspi_rd_batch(hbqspi, pbuf, residue);
	if (remainder > 0)
		ret = hb_qspi_rd_byte(hbqspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("hb_qspi_read failed!\n");

	return ret;
}

static inline int hb_qspi_write(struct hb_qspi_priv *hbqspi,
						 const void *pbuf, uint32_t len)
{
	int32_t ret = 0;
	u32 remainder = len % HB_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = hb_qspi_wr_batch(hbqspi, pbuf, residue);
	if (remainder > 0)
		ret = hb_qspi_wr_byte(hbqspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("hb_qspi_write failed!\n");

	return ret;
}

static inline int hb_qspi_xfer(struct udevice *dev, unsigned int bitlen,
						const void *dout, void *din, unsigned long flags)
{
	int ret = 0;
	uint32_t len, cs;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	if (bitlen == 0) {
		return 0;
	}
	len = bitlen / 8;
	cs	= slave_plat->cs;
	hb_qspi_set_wire(hbqspi, 1);

	if (flags & SPI_XFER_BEGIN)
		hb_qspi_wr_reg(hbqspi, HB_QSPI_CS_REG, 1 << cs);

	if (dout)
		ret = hb_qspi_write(hbqspi, dout, len);
	else if (din)
		ret = hb_qspi_read(hbqspi, din, len);

	if (flags & SPI_XFER_END)
		hb_qspi_wr_reg(hbqspi, HB_QSPI_CS_REG, 0);  /* Deassert CS after transfer */

#ifdef QSPI_DEBUG
		trace_transfer(dout ? dout : din, len, dout ? false : true);
#endif
	return ret;
}

static void hb_qspi_hw_init(struct hb_qspi_priv *hbqspi)
{
	uint32_t val;
	/* set qspi clk div */
	set_speed(plat->sclk);

	/* disable batch operation and reset fifo */
	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL3_REG);
	val |= HB_QSPI_BATCH_DIS;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL3_REG, val);
	hb_qspi_reset_fifo(hbqspi);

	/* clear status */
	val = HB_QSPI_MODF | HB_QSPI_RBD | HB_QSPI_TBD;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_ST1_REG, val);
	val = HB_QSPI_TXRD_EMPTY | HB_QSPI_RXWR_FULL;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_ST2_REG, val);

	/* set qspi work mode */
	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_CTL1_REG);
	val |= HB_QSPI_MST;
	val &= (~HB_QSPI_FW_MASK);
	val |= HB_QSPI_FW8;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL1_REG, val);

	/* Disable all interrupt */
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CTL2_REG, 0x0);

	/* unselect chip */
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CS_REG, 0x0);

	/* Always set SPI to one line as init. */
	val = 0x48;
	hb_qspi_wr_reg(hbqspi, HB_QSPI_DQM_REG, val);

	/* Disable hardware xip mode */
	val = hb_qspi_rd_reg(hbqspi, HB_QSPI_XIP_REG);
	val &= ~(1 << 1);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_XIP_REG, val);

	/* Set Rx/Tx fifo trig level  */
	hb_qspi_wr_reg(hbqspi, HB_QSPI_RTL_REG, HB_QSPI_TRIG_LEVEL);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_TTL_REG, HB_QSPI_TRIG_LEVEL);

	return;
}

/**
 * hb_qspi_exec_mem_op() - Initiates the QSPI transfer
 * @mem: the SPI memory
 * @op: the memory operation to execute
 *
 * Executes a memory operation.
 *
 * This function first selects the chip and starts the memory operation.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
static int hb_qspi_exec_mem_op(struct spi_slave *slave,
				 const struct spi_mem_op *op)
{
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(slave->dev);
	int ret = 0, non_data_size = 0, i;
	uint8_t *non_data_buf = NULL, *tmp_ptr;
#if (QSPI_DEBUG > 1)
	if (op->cmd.opcode != 0x0f) {
		printf("cmd:0x%02x addr_dum_dat_nbytes:%d %d %d bus_widths:%d%d%d%d\n",
			op->cmd.opcode,
			op->addr.nbytes, op->dummy.nbytes, op->data.nbytes,
			op->cmd.buswidth, op->addr.buswidth,
			op->dummy.buswidth, op->data.buswidth);
	}
#endif
	/* First deal with non-data transmits: cmd/addr/dummy */
	non_data_size = op->addr.nbytes + op->dummy.nbytes;
	non_data_buf = (uint8_t *) malloc(non_data_size);
	memset(non_data_buf, 0x0, non_data_size);
	tmp_ptr = non_data_buf;

	if (op->cmd.opcode) {
		memcpy(tmp_ptr, (u8 *)&op->cmd.opcode, sizeof(op->cmd.opcode));
		tmp_ptr += sizeof(op->cmd.opcode);
	}

	if (op->addr.nbytes) {
		memset(tmp_ptr, 0, sizeof(op->addr.nbytes));
		for (i = 0; i < op->addr.nbytes; i++)
			tmp_ptr[i] = op->addr.val >>
					(8 * (op->addr.nbytes - i - 1));

		tmp_ptr += op->addr.nbytes;
	}

	if (op->dummy.nbytes) {
		memset(tmp_ptr, 0xff, op->dummy.nbytes);
	}

	hb_qspi_set_wire(hbqspi, 1);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CS_REG, 1 << slave_plat->cs);

	if (op->cmd.opcode) {
		ret = hb_qspi_wr_byte(hbqspi, non_data_buf, sizeof(op->cmd.opcode));
	}

	hb_qspi_set_wire(hbqspi, op->addr.buswidth);

	ret += hb_qspi_wr_byte(hbqspi,
					non_data_buf + sizeof(op->cmd.opcode), non_data_size);

	if(ret) {
		ret = -1;
		goto exec_end;
	}

	ret = 0;
	if (op->data.nbytes) {
		if (op->data.buswidth == 4)
			hb_qspi_set_wire(hbqspi, 4);
		else if (op->data.buswidth == 2)
			hb_qspi_set_wire(hbqspi, 2);
		else
			hb_qspi_set_wire(hbqspi, 1);

		if (op->data.dir == SPI_MEM_DATA_OUT) {
			ret = hb_qspi_write(hbqspi, op->data.buf.out, op->data.nbytes);
		} else {
			ret = hb_qspi_read(hbqspi, op->data.buf.in, op->data.nbytes);
		}
	}

exec_end:
	if (non_data_buf)
		free(non_data_buf);
	hb_qspi_wr_reg(hbqspi, HB_QSPI_CS_REG, 0);
	hb_qspi_set_wire(hbqspi, 1);

	if (ret)
		printf("QSPI Exec op failed! cmd:%#02x err: %d\n",
				op->cmd.opcode, ret);
	return ret;
}

static bool hb_supports_op(struct spi_slave *slave,
						   const struct spi_mem_op *op)
{
	struct hb_qspi_platdata *slave_plat = plat;
	if (op->cmd.buswidth > 1
		|| op->addr.buswidth > slave_plat->bus_width
		|| op->dummy.buswidth > slave_plat->bus_width
		|| op->data.buswidth > slave_plat->bus_width)
		return false;

	return true;
}
static const struct spi_controller_mem_ops hb_mem_ops = {
	.supports_op = hb_supports_op,
	.exec_op = hb_qspi_exec_mem_op,
};

static int hb_qspi_ofdata_to_platdata(struct udevice *bus)
{
	int ret = 0;
	struct hb_qspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);
	struct clk hbqspi_clk;
	u32 tmp_bus_width;

	plat->regs_base = (u32 *)fdtdec_get_addr(blob, node, "reg");
	plat->sclk = fdtdec_get_uint(blob, node,
									"spi-max-frequency", HB_QSPI_DEF_BR);
	/* obtain bus width from dts */
	plat->bus_width = fdtdec_get_uint(blob, node, "spi-tx-bus-width", 1);
	tmp_bus_width = fdtdec_get_uint(blob, node, "spi-rx-bus-width", 1);
	plat->bus_width =  tmp_bus_width < plat->bus_width ?
						tmp_bus_width : plat->bus_width;

	ret = clk_get_by_index(bus, 0, &hbqspi_clk);
	if (!(ret < 0)) {
		/* plat->hclk = clk_get_rate(&hbqspi_clk); */
		/* FIXME: Since clk_get_rate does not return the correct value, use predefined Speed */
		plat->hclk = HB_QSPI_DEF_HCLK;
	}
	if ((ret < 0) || (plat->hclk <= 0)) {
		printf("Failed to get clk!\n");
		plat->hclk = fdtdec_get_int(blob, node,
									"spi-max-frequency", HB_QSPI_DEF_BR);
	}
	debug("%s:%d plat: sclk:%d hclk:%d, buswidth:%d\n", __func__, __LINE__,
				plat->sclk, plat->hclk, plat->bus_width);
	// TODO: Chip Select Define
	return 0;
}

static int hb_qspi_child_pre_probe(struct udevice *bus)
{
	/* printf("entry %s:%d\n", __func__, __LINE__); */
	return 0;
}

static int hb_qspi_probe(struct udevice *bus)
{
	plat = dev_get_platdata(bus);
	hbqspi = dev_get_priv(bus);

	hbqspi->regs_base = plat->regs_base;

	/* init the Hobot qspi hw */
	hb_qspi_hw_init(hbqspi);

	return 0;
}

static const struct dm_spi_ops hb_qspi_ops = {
	.claim_bus	 = hb_qspi_claim_bus,
	.release_bus = hb_qspi_release_bus,
	.xfer		 = hb_qspi_xfer,
	.set_speed	 = hb_qspi_set_speed,
	.set_mode	 = hb_qspi_set_mode,
	.mem_ops	 = &hb_mem_ops,
};

static const struct udevice_id hb_qspi_ids[] = {
	{ .compatible = "hb,qspi" },
	{ }
};

U_BOOT_DRIVER(qspi) = {
	.name	  = HB_QSPI_NAME,
	.id 	  = UCLASS_SPI,
	.of_match = hb_qspi_ids,
	.ops	  = &hb_qspi_ops,
	.ofdata_to_platdata 	  = hb_qspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct hb_qspi_platdata),
	.priv_auto_alloc_size	  = sizeof(struct hb_qspi_priv),
	.probe			 = hb_qspi_probe,
	.child_pre_probe = hb_qspi_child_pre_probe,
};
