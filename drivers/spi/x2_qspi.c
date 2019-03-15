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
#include "x2_qspi.h"

DECLARE_GLOBAL_DATA_PTR;

/**
 * struct x2_qspi_platdata - platform data for x2 QSPI
 *
 * @regs: Point to the base address of QSPI registers
 * @freq: QSPI input clk frequency
 * @speed_hz: Default BUS sck frequency
 * @xfer_mode: 0-Byte 1-batch 2-dma, Default work in byte mode
 */
struct x2_qspi_platdata {
	void __iomem *regs_base;
	u32 freq;
	u32 speed_hz;
};

struct x2_qspi_priv {
	void __iomem *regs_base;
};

#define x2_qspi_rd(dev, reg)	   readl((dev)->regs_base + (reg))
#define x2_qspi_wr(dev, reg, val)  writel((val), (dev)->regs_base + (reg))

static int x2_qspi_set_speed(struct udevice *bus, uint speed)
{
	int confr, prescaler, divisor;
	unsigned int max_br, min_br, br_div;
	struct x2_qspi_platdata *plat = dev_get_platdata(bus);
	struct x2_qspi_priv *x2qspi   = dev_get_priv(bus);

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
				x2_qspi_wr(x2qspi, X2_QSPI_BDR_REG, confr);
				return 0;
			}
		}
	}

	return 0;
}

static int x2_qspi_set_mode(struct udevice *bus, uint mode)
{
	unsigned int val = 0;
	struct x2_qspi_priv *x2qspi = dev_get_priv(bus);

	val = x2_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	if (mode & SPI_CPHA)
		val |= X2_QSPI_CPHA;
	if (mode & SPI_CPOL)
		val |= X2_QSPI_CPOL;
	if (mode & SPI_LSB_FIRST)
		val |= X2_QSPI_LSB;
	if (mode & SPI_SLAVE)
		val &= (~X2_QSPI_MST);
	x2_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	return 0;
}

/* currend driver only support BYTE/DUAL/QUAD for both RX and TX */
static int x2_qspi_set_wire(struct x2_qspi_priv *x2qspi, uint mode)
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
	x2_qspi_wr(x2qspi, X2_QSPI_DQM_REG, val);

	return 0;
}

static int x2_qspi_claim_bus(struct udevice *dev)
{
	/* Now x2 qspi Controller is always on */
	return 0;
}

static int x2_qspi_release_bus(struct udevice *dev)
{
	/* Now x2 qspi Controller is always on */
	return 0;
}

static void x2_qspi_reset_fifo(struct x2_qspi_priv *x2qspi)
{
	u32 val;

	val = x2_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);
	val |= X2_QSPI_RST_ALL;
	x2_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);
	mdelay(1);
	val = (~X2_QSPI_RST_ALL);
	x2_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);

	return;
}

/* tx almost empty */
static int x2_qspi_tx_ae(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(100);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TX_AE)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

/* rx almost full */
static int x2_qspi_rx_af(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(100);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_RX_AF)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int x2_qspi_tb_done(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(100);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TBD)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	x2_qspi_wr(x2qspi, X2_QSPI_ST1_REG, (val|X2_QSPI_TBD));


	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int x2_qspi_rb_done(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(100);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST1_REG);
		trys ++;
	} while ((!(val&X2_QSPI_RBD)) && (trys<TRYS_TOTAL_NUM));
	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);
	x2_qspi_wr(x2qspi, X2_QSPI_ST1_REG, (val|X2_QSPI_RBD));


	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int x2_qspi_tx_full(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(100);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((val&X2_QSPI_TX_FULL) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int x2_qspi_tx_empty(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(10);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((!(val&X2_QSPI_TX_EP)) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

static int x2_qspi_rx_empty(struct x2_qspi_priv *x2qspi)
{
	u32 val, trys=0;

	do {
		udelay(10);
		val = x2_qspi_rd(x2qspi, X2_QSPI_ST2_REG);
		trys ++;
	} while ((val&X2_QSPI_RX_EP) && (trys<TRYS_TOTAL_NUM));

	if (trys >= TRYS_TOTAL_NUM)
		printf("%s_%d:val=%x, trys=%d\n", __func__, __LINE__, val, trys);

	return trys<TRYS_TOTAL_NUM ? 0 : -1;
}

/* config fifo width */
static void x2_qspi_set_fw(struct x2_qspi_priv *x2qspi, u32 fifo_width)
{
	u32 val;

	val = x2_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	val &= (~0x3);
	val |= fifo_width;
	x2_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	return;
}

/*config xfer mode:enable/disable BATCH/RX/TX */
static void x2_qspi_set_xfer(struct x2_qspi_priv *x2qspi, u32 op_flag)
{
	u32 ctl1_val = 0, ctl3_val = 0;

	ctl1_val = x2_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	ctl3_val = x2_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);

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
	x2_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, ctl1_val);
	x2_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, ctl3_val);

	return;
}

static int x2_qspi_rd_batch(struct x2_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 i, rx_len, offset = 0, tmp_len = len, ret = 0;
	u32 *dbuf = (u32 *) pbuf;

	/* Enable batch mode */
	x2_qspi_set_fw(x2qspi, X2_QSPI_FW32);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		rx_len = MIN(tmp_len, BATCH_MAX_CNT);
		x2_qspi_wr(x2qspi, X2_QSPI_RBC_REG, rx_len);
		/* enbale rx */
		x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_EN);

		for (i=0; i< rx_len; i+=8) {
			if (x2_qspi_rx_af(x2qspi)) {
				ret = -1;
				goto rb_err;
			}
			dbuf[offset++] = x2_qspi_rd(x2qspi, X2_QSPI_DAT_REG);
			dbuf[offset++] = x2_qspi_rd(x2qspi, X2_QSPI_DAT_REG);
		}
		if (x2_qspi_rb_done(x2qspi)) {
			printf("%s_%d:rx failed! len=%d, received=%d, i=%d\n", __func__, __LINE__, len, offset, i);
			ret = -1;
			goto rb_err;
		}
		x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);
		tmp_len = tmp_len - rx_len;
	}

rb_err:
	/* Disable batch mode and rx link */
	x2_qspi_set_fw(x2qspi, X2_QSPI_FW8);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_DIS);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);

	return ret;
}

static int x2_qspi_wr_batch(struct x2_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 i, tx_len, offset = 0, tmp_len = len, ret = 0;
	u32 *dbuf = (u32 *) pbuf;

	x2_qspi_set_fw(x2qspi, X2_QSPI_FW32);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_EN);

	while (tmp_len > 0) {
		tx_len = MIN(tmp_len, BATCH_MAX_CNT);
		x2_qspi_wr(x2qspi, X2_QSPI_TBC_REG, tx_len);

		/* enbale tx */
		x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_EN);

		for (i=0; i<tx_len; i+=8) {
			if (x2_qspi_tx_ae(x2qspi)) {
				ret = -1;
				goto tb_err;
			}
			x2_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[offset++]);
			x2_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[offset++]);
		}
		if (x2_qspi_tb_done(x2qspi)) {
			printf("%s_%d:tx failed! len=%d, received=%d, i=%d\n", __func__, __LINE__, len, offset, i);
			ret = -1;
			goto tb_err;
		}
		tmp_len = tmp_len - tx_len;
	}

tb_err:
	/* Disable batch mode and tx link */
	x2_qspi_set_fw(x2qspi, X2_QSPI_FW8);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_BAT_DIS);
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_DIS);

	return ret;
}

static int x2_qspi_rd_byte(struct x2_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale rx */
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_EN);

	for (i=0; i<len; i++)
	{
		if (x2_qspi_tx_empty(x2qspi)) {
			ret = -1;
			goto rd_err;
		}
		x2_qspi_wr(x2qspi, X2_QSPI_DAT_REG, 0x00);
		if (x2_qspi_rx_empty(x2qspi)) {
			ret = -1;
			goto rd_err;
		}
		dbuf[i] = x2_qspi_rd(x2qspi, X2_QSPI_DAT_REG) & 0xFF;
	}

rd_err:
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_RX_DIS);

	if (0 != ret)
		printf("%s_%d:read op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static int x2_qspi_wr_byte(struct x2_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 i, ret = 0;
	u8 *dbuf = (u8 *) pbuf;

	/* enbale tx */
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_EN);

	for (i=0; i<len; i++)
	{
		if (x2_qspi_tx_full(x2qspi)) {
			ret = -1;
			goto wr_err;
		}
		x2_qspi_wr(x2qspi, X2_QSPI_DAT_REG, dbuf[i]);
	}
	/* Check tx complete */
	if (x2_qspi_tx_empty(x2qspi))
		ret = -1;

wr_err:
	x2_qspi_set_xfer(x2qspi, X2_QSPI_OP_TX_DIS);

	if (0 != ret)
		printf("%s_%d:write op failed! i=%d\n", __func__, __LINE__, i);

	return ret;
}

static int x2_qspi_read(struct x2_qspi_priv *x2qspi, void *pbuf, uint32_t len)
{
	u32 ret = 0;
	u32 remainder = len % X2_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = x2_qspi_rd_batch(x2qspi, pbuf, residue);
	if (remainder > 0)
		ret = x2_qspi_rd_byte(x2qspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("x2_qspi_read failed!\n");

	return ret;
}

static int x2_qspi_write(struct x2_qspi_priv *x2qspi, const void *pbuf, uint32_t len)
{
	u32 ret = 0;
	u32 remainder = len % X2_QSPI_TRIG_LEVEL;
	u32 residue   = len - remainder;

	if (residue > 0)
		ret = x2_qspi_wr_batch(x2qspi, pbuf, residue);
	if (remainder > 0)
		ret = x2_qspi_wr_byte(x2qspi, (u8 *) pbuf + residue, remainder);
	if (ret < 0)
		printf("x2_qspi_write failed!\n");

	return ret;
}

int x2_qspi_xfer(struct udevice *dev, unsigned int bitlen, const void *dout,
		 void *din, unsigned long flags)
{
	int ret = 0;
	unsigned int len, cs;
	struct udevice *bus = dev->parent;
	struct x2_qspi_priv *x2qspi = dev_get_priv(bus);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	if (bitlen == 0) {
		return 0;
	}
	len = bitlen / 8;
	cs	= slave_plat->cs;

	if (flags & SPI_XFER_BEGIN)
		x2_qspi_wr(x2qspi, X2_QSPI_CS_REG, 1<<cs);  /* Assert CS before transfer */

	if (dout) {
		ret = x2_qspi_write(x2qspi, dout, len);
	}
	else if (din) {
		ret = x2_qspi_read(x2qspi, din, len);
	}

	if (flags & SPI_XFER_END) {
		x2_qspi_wr(x2qspi, X2_QSPI_CS_REG, 0);  /* Deassert CS after transfer */
	}

	if (flags & SPI_XFER_CMD) {
		switch (((u8 *) dout) [0]) {
		case CMD_READ_QUAD_OUTPUT_FAST:
		case CMD_QUAD_PAGE_PROGRAM:
		case CMD_READ_DUAL_OUTPUT_FAST:
			x2_qspi_set_wire(x2qspi, slave_plat->mode);
			break;
		}
	} else {
		x2_qspi_set_wire(x2qspi, 0);
	}

	return ret;
}

static void x2_qspi_hw_init(struct x2_qspi_priv *x2qspi)
{
	uint32_t val;

	/* disable batch operation and reset fifo */
	val = x2_qspi_rd(x2qspi, X2_QSPI_CTL3_REG);
	val |= X2_QSPI_BATCH_DIS;
	x2_qspi_wr(x2qspi, X2_QSPI_CTL3_REG, val);
	x2_qspi_reset_fifo(x2qspi);

	/* clear status */
	val = X2_QSPI_MODF | X2_QSPI_RBD | X2_QSPI_TBD;
	x2_qspi_wr(x2qspi, X2_QSPI_ST1_REG, val);
	val = X2_QSPI_TXRD_EMPTY | X2_QSPI_RXWR_FULL;
	x2_qspi_wr(x2qspi, X2_QSPI_ST2_REG, val);

	/* set qspi work mode */
	val = x2_qspi_rd(x2qspi, X2_QSPI_CTL1_REG);
	val |= X2_QSPI_MST;
	val &= (~X2_QSPI_FW_MASK);
	val |= X2_QSPI_FW8;
	x2_qspi_wr(x2qspi, X2_QSPI_CTL1_REG, val);

	/* Disable all interrupt */
	x2_qspi_wr(x2qspi, X2_QSPI_CTL2_REG, 0x0);

	/* unselect chip */
	x2_qspi_wr(x2qspi, X2_QSPI_CS_REG, 0x0);

	/* Always set SPI to one line as init. */
	val = x2_qspi_rd(x2qspi, X2_QSPI_DQM_REG);
	val |= 0xfc;
	x2_qspi_wr(x2qspi, X2_QSPI_DQM_REG, val);

	/* Disable hardware xip mode */
	val = x2_qspi_rd(x2qspi, X2_QSPI_XIP_REG);
	val &= ~(1 << 1);
	x2_qspi_wr(x2qspi, X2_QSPI_XIP_REG, val);

	/* Set Rx/Tx fifo trig level  */
	x2_qspi_wr(x2qspi, X2_QSPI_RTL_REG, X2_QSPI_TRIG_LEVEL);
	x2_qspi_wr(x2qspi, X2_QSPI_TTL_REG, X2_QSPI_TRIG_LEVEL);

	return;
}
static int x2_qspi_ofdata_to_platdata(struct udevice *bus)
{
	struct x2_qspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);

	plat->regs_base = (u32 *) fdtdec_get_addr(blob, node, "reg");
	plat->freq		= fdtdec_get_int(blob, node, "spi-max-frequency", 500000000);

	return 0;
}

static int x2_qspi_child_pre_probe(struct udevice *bus)
{
	/* printf("entry %s:%d\n", __func__, __LINE__); */
	return 0;
}

static int x2_qspi_probe(struct udevice *bus)
{
	struct x2_qspi_platdata *plat = dev_get_platdata(bus);
	struct x2_qspi_priv *priv	  = dev_get_priv(bus);

	priv->regs_base = plat->regs_base;

	/* init the x2 qspi hw */
	x2_qspi_hw_init(priv);

	return 0;
}

static const struct dm_spi_ops x2_qspi_ops = {
	.claim_bus	 = x2_qspi_claim_bus,
	.release_bus = x2_qspi_release_bus,
	.xfer		 = x2_qspi_xfer,
	.set_speed	 = x2_qspi_set_speed,
	.set_mode	 = x2_qspi_set_mode,
};

static const struct udevice_id x2_qspi_ids[] = {
	{ .compatible = "x2,qspi" },
	{ }
};

U_BOOT_DRIVER(qspi) = {
	.name	  = X2_QSPI_NAME,
	.id 	  = UCLASS_SPI,
	.of_match = x2_qspi_ids,
	.ops	  = &x2_qspi_ops,
	.ofdata_to_platdata 	  = x2_qspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct x2_qspi_platdata),
	.priv_auto_alloc_size	  = sizeof(struct x2_qspi_priv),
	.probe			 = x2_qspi_probe,
	.child_pre_probe = x2_qspi_child_pre_probe,
};
