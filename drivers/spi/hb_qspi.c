// SPDX-License-Identifier: GPL-2.0+
/*
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
#include "hb_qspi.h"

DECLARE_GLOBAL_DATA_PTR;

/**
 * struct hb_qspi_platdata - platform data for x2 QSPI
 *
 * @regs: Point to the base address of QSPI registers
 * @freq: QSPI input clk frequency
 * @speed_hz: Default BUS sck frequency
 * @xfer_mode: 0-Byte 1-batch 2-dma, Default work in byte mode
 */
struct hb_qspi_platdata {
	void __iomem *regs_base;
	u32 freq;
	u32 speed_hz;
};

struct hb_qspi_priv {
	void __iomem *regs_base;
};

#define hb_qspi_rd(dev, reg)	   readl((dev)->regs_base + (reg))
#define hb_qspi_wr(dev, reg, val)  writel((val), (dev)->regs_base + (reg))

static int hb_qspi_set_speed(struct udevice *bus, uint speed)
{
	int confr, prescaler, divisor;
	unsigned int max_br, min_br, br_div;
	struct hb_qspi_platdata *plat = dev_get_platdata(bus);
	struct hb_qspi_priv *x2qspi   = dev_get_priv(bus);

	max_br = plat->freq / 2;
	min_br = plat->freq / 1048576;
	if (speed > max_br) {
		speed = max_br;
		printf("Warning:speed[%d] > max_br[%d],speed will be set to max_br\n", speed, max_br);
	}
	if (speed < min_br) {
		speed = min_br;
		printf("Warning:speed[%d] < min_br[%d],speed will be set to min_br\n", speed, min_br);
	}

	for (prescaler = 15; prescaler >= 0; prescaler--) {
		for (divisor = 15; divisor >= 0; divisor--) {
			br_div = (prescaler + 1) * (2 << divisor);
			if ((plat->freq / br_div) >= speed) {
				confr = (prescaler | (divisor << 4)) & 0xFF;

				hb_qspi_wr(x2qspi, X2_QSPI_BDR_REG, confr);
				return 0;
			}
		}
	}

	return 0;
}

static int hb_qspi_set_mode(struct udevice *bus, uint mode)
{
	unsigned int val = 0;
	struct hb_qspi_priv *x2qspi = dev_get_priv(bus);

	val = hb_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	if (mode & SPI_CPHA)
		val |= X2_QSPI_CPHA;
	if (mode & SPI_CPOL)
		val |= X2_QSPI_CPOL;
	if (mode & SPI_LSB_FIRST)
		val |= X2_QSPI_LSB;
	if (mode & SPI_SLAVE)
		val &= (~X2_QSPI_MST);
	hb_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	return 0;
}

/* currend driver only support BYTE/DUAL/QUAD for both RX and TX */
static int hb_qspi_set_wire(struct hb_qspi_priv *x2qspi, uint mode)
{
	unsigned int val = 0;

	switch (mode & 0x3F00) {
	case SPI_TX_BYTE | SPI_RX_SLOW:
		val = 0xFC;
		break;
	case SPI_TX_DUAL | SPI_RX_DUAL:
		val = X2_QSPI_DUAL;
		break;
	case SPI_TX_QUAD | SPI_RX_QUAD:
		val = X2_QSPI_QUAD;
		break;
	default:
		val = 0xFC;
		break;
	}
	hb_qspi_wr(x2qspi, X2_QSPI_DQM_REG, val);

	return 0;
}

static int hb_qspi_claim_bus(struct udevice *dev)
{
	/* Now x2 qspi Controller is always on */
	return 0;
}

static int hb_qspi_release_bus(struct udevice *dev)
{
	/* Now x2 qspi Controller is always on */
	return 0;
}

static void hb_qspi_reset_fifo(struct hb_qspi_priv *x2qspi)
{
	u32 val;

	val = hb_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);
	val |= X2_QSPI_RST_ALL;
	hb_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);
	mdelay(1);
	val = (~X2_QSPI_RST_ALL);
	hb_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);

	return;
}

/* tx almost empty */
static int hb_qspi_tx_ae(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TX_AE)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

/* rx almost full */
static int hb_qspi_rx_af(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_RX_AF)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int hb_qspi_tb_done(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TBD)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	hb_qspi_wr(x2qspi, X2_QSPI_ST1_REG, (val|X2_QSPI_TBD));


	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int hb_qspi_rb_done(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_RBD)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	hb_qspi_wr(x2qspi, X2_QSPI_ST1_REG, (val|X2_QSPI_RBD));


	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int hb_qspi_tx_full(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((val&X2_QSPI_TX_FULL) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int hb_qspi_tx_empty(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TX_EP)) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int hb_qspi_rx_empty(struct hb_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		ndelay(10);
		val = hb_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((val&X2_QSPI_RX_EP) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

/* config fifo width */
static void hb_qspi_set_fw(struct hb_qspi_priv *x2qspi, u32 fifo_width)
{
	u32 val;

	val = hb_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	val &= (~0x3);
	val |= fifo_width;
	hb_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	return;
}

/*config xfer mode:enable/disable BATCH/RX/TX */
static void hb_qspi_set_xfer(struct hb_qspi_priv *x2qspi, u32 op_flag)
{
	u32 ctl1_val = 0, ctl3_val = 0;

	ctl1_val = hb_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	ctl3_val = hb_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);

	switch (op_flag) {
	case X2_QSPI_OP_RX_EN:
		ctl1_val |= X2_QSPI_RX_EN;
		break;
	case X2_QSPI_OP_RX_DIS:
		ctl1_val &= (~X2_QSPI_RX_EN);
		break;
	case X2_QSPI_OP_TX_EN:
		ctl1_val |= X2_QSPI_TX_EN;
		break;
	case X2_QSPI_OP_TX_DIS:
		ctl1_val &= (~X2_QSPI_TX_EN);
		break;
	case X2_QSPI_OP_BAT_EN:
		ctl3_val &= (~X2_QSPI_BATCH_DIS);
		break;
	case X2_QSPI_OP_BAT_DIS:
		ctl3_val |= X2_QSPI_BATCH_DIS;
		break;
	default:
		printf("Op(0x%x) if error, please check it!\n", op_flag);
		break;
	}
	hb_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, ctl1_val);
	hb_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, ctl3_val);

	return;
}

static int hb_qspi_rd_batch(struct hb_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 i, rx_len, offset = 0, tmp_len = len, ret = 0;
	u32 *dbuf = (u32 *) pbuf;

	/* Enable batch mode */
	hb_qspi_set_fw(x2qspi, X2_QSPI_FW32);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		rx_len = MIN(tmp_len, BATCH_MAX_CNT);
		hb_qspi_wr(x2qspi, X2_QSPI_RBC_REG, rx_len);
		/* enbale rx */
		hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_EN);

		for (i=0; i< rx_len; i+=8) {
			if (hb_qspi_rx_af(x2qspi)) {
				ret = -1;
				goto rb_err;
			}
			dbuf[offset++] = hb_qspi_rd(x2qspi, X2_QSPI_DAT_REG);
			dbuf[offset++] = hb_qspi_rd(x2qspi, X2_QSPI_DAT_REG);
		}
		if (hb_qspi_rb_done(x2qspi)) {
			printf("%s_%d:rx failed! len=%d, received=%d, i=%d\n", __func__, __LINE__, len, offset, i);
			ret = -1;
			goto rb_err;
		}
		hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);
		tmp_len = tmp_len - rx_len;
	}

rb_err:
	/* Disable batch mode and rx link */
	hb_qspi_set_fw(x2qspi, X2_QSPI_FW8);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_DIS);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);

	return ret;
}

static int hb_qspi_wr_batch(struct hb_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 i, tx_len, offset = 0, tmp_len = len, ret = 0;
	u32 *dbuf = (u32 *) pbuf;

	hb_qspi_set_fw(x2qspi, X2_QSPI_FW32);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		tx_len = MIN(tmp_len, BATCH_MAX_CNT);
		hb_qspi_wr(x2qspi, X2_QSPI_TBC_REG, tx_len);

		/* enbale tx */
		hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_EN);

		for (i=0; i<tx_len; i+=8) {
			if (hb_qspi_tx_ae(x2qspi)) {
				ret = -1;
				goto tb_err;
			}
			hb_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[offset++]);
			hb_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[offset++]);
		}
		if (hb_qspi_tb_done(x2qspi)) {
			printf("%s_%d:tx failed! len=%d, received=%d, i=%d\n", __func__, __LINE__, len, offset, i);
			ret = -1;
			goto tb_err;
		}
		tmp_len = tmp_len - tx_len;
	}

tb_err:
	/* Disable batch mode and tx link */
	hb_qspi_set_fw(x2qspi, X2_QSPI_FW8);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_DIS);
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_DIS);

	return ret;
}

static int hb_qspi_rd_byte(struct hb_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale rx */
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_EN);

	for (i=0; i<len; i++)
	{
		if (hb_qspi_tx_empty(x2qspi)) {
			ret = -1;
			goto rd_err;
		}
		hb_qspi_wr(x2qspi, X2_QSPI_DAT_REG, 0x00);
		if (hb_qspi_rx_empty(x2qspi)) {
			ret = -1;
			goto rd_err;
		}
		dbuf[i] = hb_qspi_rd(x2qspi, X2_QSPI_DAT_REG) & 0xFF;
	}

rd_err:
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);

	if (0 != ret)
		printf("%s_%d:read op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static int hb_qspi_wr_byte(struct hb_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale tx */
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_EN);

	for (i=0; i<len; i++)
	{
		if (hb_qspi_tx_full(x2qspi)) {
			ret = -1;
			goto wr_err;
		}
		hb_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[i]);
	}
	/* Check tx complete */
	if (hb_qspi_tx_empty(x2qspi))
		ret = -1;

wr_err:
	hb_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_DIS);

	if (0 != ret)
		printf("%s_%d:write op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static int hb_qspi_read(struct hb_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 ret = 0;
	u32 remainder = len % X2_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = hb_qspi_rd_batch(x2qspi, pbuf, residue);
	if (remainder > 0)
		ret = hb_qspi_rd_byte(x2qspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("hb_qspi_read failed!\n");

	return ret;
}

static int hb_qspi_write(struct hb_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 ret = 0;
	u32 remainder = len % X2_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = hb_qspi_wr_batch(x2qspi, pbuf, residue);
	if (remainder > 0)
		ret = hb_qspi_wr_byte(x2qspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("hb_qspi_write failed!\n");

	return ret;
}

int hb_qspi_xfer(struct udevice *dev, unsigned int bitlen, const void *dout,
		 void *din, unsigned long flags)
{
	int ret = 0;
	unsigned int len, cs;
	struct udevice *bus = dev->parent;
	struct hb_qspi_priv *x2qspi = dev_get_priv(bus);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	if (bitlen == 0) {
		return 0;
	}
	len = bitlen / 8;
	cs	= slave_plat->cs;

	if (flags & SPI_XFER_BEGIN)
		hb_qspi_wr(x2qspi, X2_QSPI_CS_REG, 1<<cs);  /* Assert CS before transfer */

	if (dout) {
		ret = hb_qspi_write(x2qspi, dout, len);
	}
	else if (din) {
		ret = hb_qspi_read(x2qspi, din, len);
	}

	if (flags & SPI_XFER_END) {
		hb_qspi_wr(x2qspi, X2_QSPI_CS_REG, 0);  /* Deassert CS after transfer */
	}

	if (flags & SPI_XFER_CMD) {
		switch (((u8 *) dout) [0]) {
		case CMD_READ_QUAD_OUTPUT_FAST:
		case CMD_QUAD_PAGE_PROGRAM:
		case CMD_READ_DUAL_OUTPUT_FAST:
			hb_qspi_set_wire(x2qspi, slave_plat->mode);
			break;
		}
	} else {
		hb_qspi_set_wire(x2qspi, 0);
	}

	return ret;
}

static void hb_qspi_hw_init(struct hb_qspi_priv *x2qspi)
{
	uint32_t val;

	/* disable batch operation and reset fifo */
	val = hb_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);
	val |= X2_QSPI_BATCH_DIS;
	hb_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);
	hb_qspi_reset_fifo(x2qspi);

	/* clear status */
	val = X2_QSPI_MODF | X2_QSPI_RBD | X2_QSPI_TBD;
	hb_qspi_wr(x2qspi, X2_QSPI_ST1_REG, val);
	val = X2_QSPI_TXRD_EMPTY | X2_QSPI_RXWR_FULL;
	hb_qspi_wr(x2qspi, X2_QSPI_ST2_REG, val);

	/* set qspi work mode */
	val = hb_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	val |= X2_QSPI_MST;
	val &= (~X2_QSPI_FW_MASK);
	val |= X2_QSPI_FW8;
	hb_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	/* Disable all interrupt */
	hb_qspi_wr(x2qspi, X2_QSPI_CTL2_REG, 0x0);

	/* unselect chip */
	hb_qspi_wr(x2qspi, X2_QSPI_CS_REG, 0x0);

	/* Always set SPI to one line as init. */
	val = hb_qspi_rd(x2qspi, X2_QSPI_DQM_REG);
	val |= 0xfc;
	hb_qspi_wr(x2qspi, X2_QSPI_DQM_REG, val);

	/* Disable hardware xip mode */
	val = hb_qspi_rd(x2qspi, X2_QSPI_XIP_REG);
	val &= ~(1 << 1);
	hb_qspi_wr(x2qspi, X2_QSPI_XIP_REG, val);

	/* Set Rx/Tx fifo trig level  */
	hb_qspi_wr(x2qspi, X2_QSPI_RTL_REG, X2_QSPI_TRIG_LEVEL);
	hb_qspi_wr(x2qspi, X2_QSPI_TTL_REG, X2_QSPI_TRIG_LEVEL);

	return;
}
static int hb_qspi_ofdata_to_platdata(struct udevice *bus)
{
	int ret = 0;
	struct hb_qspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);
	struct clk x2qspi_clk;

	plat->regs_base = (u32 *)fdtdec_get_addr(blob, node, "reg");

	ret = clk_get_by_index(bus, 0, &x2qspi_clk);
	if (!(ret < 0)) {
		plat->freq = clk_get_rate(&x2qspi_clk);
	}
	if ((ret < 0) || (plat->freq <= 0)) {
		printf("Failed to get clk!\n");
		plat->freq = fdtdec_get_int(blob, node, "spi-max-frequency", 500000000);
	}

	return 0;
}

static int hb_qspi_child_pre_probe(struct udevice *bus)
{
	/* printf("entry %s:%d\n", __func__, __LINE__); */
	return 0;
}

static int hb_qspi_probe(struct udevice *bus)
{
	struct hb_qspi_platdata *plat = dev_get_platdata(bus);
	struct hb_qspi_priv *priv	  = dev_get_priv(bus);

	priv->regs_base = plat->regs_base;

	/* init the x2 qspi hw */
	hb_qspi_hw_init(priv);

	return 0;
}

static const struct dm_spi_ops hb_qspi_ops = {
	.claim_bus	 = hb_qspi_claim_bus,
	.release_bus = hb_qspi_release_bus,
	.xfer		 = hb_qspi_xfer,
	.set_speed	 = hb_qspi_set_speed,
	.set_mode	 = hb_qspi_set_mode,
};

static const struct udevice_id hb_qspi_ids[] = {
	{ .compatible = "hb,qspi" },
	{ }
};

U_BOOT_DRIVER(qspi) = {
	.name	  = X2_QSPI_NAME,
	.id 	  = UCLASS_SPI,
	.of_match = hb_qspi_ids,
	.ops	  = &hb_qspi_ops,
	.ofdata_to_platdata 	  = hb_qspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct hb_qspi_platdata),
	.priv_auto_alloc_size	  = sizeof(struct hb_qspi_priv),
	.probe			 = hb_qspi_probe,
	.child_pre_probe = hb_qspi_child_pre_probe,
};
