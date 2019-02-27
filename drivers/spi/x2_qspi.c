
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

/* QSPI register offsets */
struct x2_qspi_regs {
    u32 tx_rx_dat;          /* 0x00 Transmit data buffer and receive data buffer */
    u32 sclk_con;           /* 0x04  Baud-rate control while working as master */
    u32 spi_ctl1;           /* 0x08 SPI work mode configuration */
    u32 spi_ctl2;           /* 0x0c SPI interrupt enabled */
    u32 spi_ctl3;           /* 0x10 SPI software reset and DMA enable */
    u32 cs;                 /* 0x14 Control the device select output */
    u32 status1;            /* 0x18 spi status reg1 */
    u32 status2;            /* 0x1c spi status reg2 */
    u32 batch_cnt_rx;       /* 0x20 Data number for RX batch transfer */
    u32 batch_cnt_tx;       /* 0x24 Data number for TX batch transfer */
    u32 rx_fifo_trig_lvl;   /* 0x28 RX FIFO trigger level register */
    u32 tx_fifo_trig_lvl;   /* 0x2C TX FIFO trigger level register */
    u32 dual_quad_mode;     /* 0x30 Dual, Quad flash operation enable register */
    u32 xip_cfg;            /* 0x34 XIP config register */
};

/**
 * struct x2_qspi_platdata - platform data for x2 QSPI
 *
 * @regs: Point to the base address of QSPI registers
 * @freq: QSPI input clk frequency
 * @speed_hz: Default BUS sck frequency
 * @xfer_mode: 0-Byte 1-batch 2-dma, Default work in byte mode
 */
struct x2_qspi_platdata {
    struct x2_qspi_regs *regs;
    u32 freq;
    u32 speed_hz;
    unsigned int xfer_mode;
};

struct x2_qspi_priv {
    struct x2_qspi_regs *regs;
    unsigned int spi_mode;
    unsigned int xfer_mode;
};

static void x2_qspi_hw_init(struct x2_qspi_priv *priv)
{
    uint32_t reg_val;
    struct x2_qspi_regs *regs = priv->regs;

    /* set qspi work mode */
    reg_val = readl(&regs->spi_ctl1);
    reg_val = 0x4;
    writel(reg_val, &regs->spi_ctl1);

    writel(FIFO_DEPTH / 2, &regs->rx_fifo_trig_lvl);
    writel(FIFO_DEPTH / 2, &regs->tx_fifo_trig_lvl);

    /* Disable all interrupt */
    writel(0x0, &regs->spi_ctl2);

    /* unselect chip */
    writel(0x0, &regs->cs);

    /* Always set SPI to one line as init. */
    reg_val = readl(&regs->dual_quad_mode);
    reg_val |= 0xfc;
    writel(reg_val, &regs->dual_quad_mode);

    /* Disable hardware xip mode */
    reg_val = readl(&regs->xip_cfg);
    reg_val &= ~(1 << 1);
    writel(reg_val, &regs->xip_cfg);

    return;
}

static int x2_qspi_child_pre_probe(struct udevice *bus)
{
    printf("entry %s:%d\n", __func__, __LINE__);
    return 0;
}

static int x2_qspi_ofdata_to_platdata(struct udevice *bus)
{
    struct x2_qspi_platdata *plat = bus->platdata;
    const void *blob = gd->fdt_blob;
    int node = dev_of_offset(bus);

    plat->regs = (struct x2_qspi_regs *)fdtdec_get_addr(blob, node, "reg");

    /* FIXME: Use 20MHz as a suitable default for FPGA board*/
    plat->freq      = fdtdec_get_int(blob, node, "spi-max-frequency", 20000000);
    plat->xfer_mode = fdtdec_get_int(blob, node, "xfer-mode", 0);

    return 0;
}
static int x2_qspi_probe(struct udevice *bus)
{
    struct x2_qspi_platdata *plat = dev_get_platdata(bus);
    struct x2_qspi_priv *priv     = dev_get_priv(bus);

    priv->regs      = plat->regs;
    priv->xfer_mode = plat->xfer_mode;

    /* init the x2 qspi hw */
    x2_qspi_hw_init(priv);

    return 0;
}

static int x2_qspi_set_speed(struct udevice *bus, uint speed)
{
    int confr, i, j;
    unsigned int max_br, min_br, br_div;
    struct x2_qspi_platdata *plat = dev_get_platdata(bus);
    struct x2_qspi_priv *priv     = dev_get_priv(bus);
    struct x2_qspi_regs *regs     = priv->regs;

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

    for (i = 15; i >= 0; i--) {
        for (j = 15; j >= 0; j--) {
            br_div = (i + 1) * (2 << j);
            if ((plat->freq / br_div) >= speed) {
                confr = (i | (j << 4)) & 0xFF;
                writel(confr, &regs->sclk_con);
                return 0;
            }
        }
    }

    return 0;
}

static int x2_qspi_set_mode(struct udevice *bus, uint mode)
{
    unsigned int confr = 0;
    struct x2_qspi_priv *priv     = dev_get_priv(bus);
    struct x2_qspi_regs *regs     = priv->regs;

    confr = readl(&regs->spi_ctl1);
    if (mode & SPI_CPHA)
        confr |= X2_QSPI_CPHA;
    if (mode & SPI_CPOL)
        confr |= X2_QSPI_CPOL;
    if (mode & SPI_LSB_FIRST)
        confr |= X2_QSPI_LSB;
    if (mode & SPI_SLAVE)
        confr &= (~X2_QSPI_MST);
    writel(confr, &regs->spi_ctl1);

    return 0;
}

/* currend driver only support BYTE/DUAL/QUAD for both RX and TX */
static int x2_qspi_set_wire(struct x2_qspi_regs *regs, uint mode)
{
    unsigned int confr = 0;

    switch(mode & 0x3F00) {
    case SPI_TX_BYTE | SPI_RX_SLOW:
        confr = 0xFC;
        break;
    case SPI_TX_DUAL | SPI_RX_DUAL:
        confr = DPI_ENABLE;
        break;
    case SPI_TX_QUAD | SPI_RX_QUAD:
        confr = QPI_ENABLE;
        break;
    default:
        confr = 0xFC;
        break;
    }
    writel(confr, &regs->dual_quad_mode);

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

int x2_qspi_poll_rx_empty(uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    uint32_t reg_val;
    uint32_t ret = -1;
    while (timeout--) {
        reg_val = readl(reg);
        if (!(reg_val & mask)) {
            ret = 0;
            break;
        }
    }
    return ret;
}


static int qspi_check_status(uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    int ret = 0;
    uint32_t val;

    do {
        val = readl(reg);
        timeout = timeout - 1;
        if (timeout == 0) {
            ret = -1;
            break;
        }
    } while (!(val & mask));

    return ret;
}

void qspi_enable_tx(uint32_t *reg)
{
    uint32_t val;
    val = readl(reg);
    val |= TX_ENABLE;
    writel(val, reg);
}

void qspi_disable_tx(uint32_t *reg)
{
    uint32_t val;
    val = readl(reg);
    val &= ~TX_ENABLE;
    writel(val, reg);
}

void qspi_enable_rx(uint32_t *reg)
{
    uint32_t val;
    val = readl(reg);
    val |= RX_ENABLE;
    writel(val, reg);
}

void qspi_disable_rx(uint32_t *reg)
{
    uint32_t val;
    val = readl(reg);
    val &= ~RX_ENABLE;
    writel(val, reg);
}

void qspi_batch_mode_set(uint32_t *reg_addr, bool enable)
{
    uint32_t val;
    val = readl(reg_addr);
    if (enable)
        val &= ~BATCH_DISABLE;
    else
        val |= BATCH_DISABLE;
    writel(val, reg_addr);
}

void qspi_batch_tx_rx_flag_clr(uint32_t *reg_addr, bool is_tx)
{
    uint32_t val;
    val = readl(reg_addr);
    if (is_tx)
        val |= BATCH_TXDONE;
    else
        val |= BATCH_RXDONE;
    writel(val, reg_addr);
}

void qspi_reset_fifo(uint32_t *reg)
{
    uint32_t val;

    val = readl(reg);
    val |= FIFO_RESET;
    writel(val, reg);

    val &= (~FIFO_RESET);
    writel(val, reg);
}

int x2_qspi_write(struct x2_qspi_priv *priv, const void *pbuf, uint32_t len)
{
    uint32_t tx_len;
    uint32_t i;
    const uint8_t *ptr = (const uint8_t *)pbuf;
    uint32_t timeout = 0x1000;
    int32_t err;
    uint32_t tmp_txlen;
    struct x2_qspi_regs *regs = priv->regs;

    qspi_disable_tx(&regs->spi_ctl1);
    qspi_reset_fifo(&regs->spi_ctl3);

    if (priv->xfer_mode == X2_QSPI_XFER_BATCH) {
        /* Enable batch mode */
        qspi_batch_mode_set(&regs->spi_ctl3, 1);

        while (len > 0) {
            tx_len = MIN(len, BATCH_MAX_SIZE);

            /* clear BATCH_TXDONE bit */
            qspi_batch_tx_rx_flag_clr(&regs->status1, 1);

            /* set batch cnt */
            writel(tx_len, &regs->batch_cnt_tx);
            tmp_txlen = MIN(tx_len, FIFO_DEPTH);
            for (i = 0; i < tmp_txlen; i++)
                writel(ptr[i], &regs->tx_rx_dat);

            qspi_enable_tx(&regs->spi_ctl1);
            err = qspi_check_status(&regs->status2, TXFIFO_EMPTY, timeout);
            if (err) {
                printf("%s:%d qspi send data timeout\n", __func__, __LINE__);
                goto SPI_ERROR;
            }

            qspi_disable_tx(&regs->spi_ctl1);
            len -= tmp_txlen;
            ptr += tmp_txlen;
        }
        /* clear BATCH_TXDONE bit */
        qspi_batch_tx_rx_flag_clr(&regs->status1, 1);

        qspi_batch_mode_set(&regs->spi_ctl3, 0);
    } else {
        /* Disable batch mode */
        qspi_batch_mode_set(&regs->spi_ctl3, 0);

        while (len > 0) {
            tx_len = len < FIFO_DEPTH ? len : FIFO_DEPTH;

            for (i = 0; i < tx_len; i++) {
                writel(ptr[i], &regs->tx_rx_dat);
            }

            qspi_enable_tx(&regs->spi_ctl1);
            err = qspi_check_status(&regs->status2, TXFIFO_EMPTY, timeout);
            if (err) {
                printf("%s:%d qspi send data timeout\n", __func__, __LINE__);
                goto SPI_ERROR;
            }

            qspi_disable_tx(&regs->spi_ctl1);
            len -= tx_len;
            ptr += tx_len;
        }
    }
    return 0;

SPI_ERROR:
    qspi_disable_tx(&regs->spi_ctl1);
    qspi_reset_fifo(&regs->spi_ctl3);
    if (priv->xfer_mode == X2_QSPI_XFER_BATCH)
        qspi_batch_mode_set(&regs->spi_ctl3, 0);
    printf("error: qspi tx %x\n", err);
    return err;
}

static int x2_qspi_read(struct x2_qspi_priv *priv, uint8_t *pbuf, uint32_t len)
{
    int32_t i = 0;
    uint32_t rx_len = 0;
    uint8_t *ptr = pbuf;
    uint32_t level;
    uint32_t rx_remain = len;
    uint32_t timeout = 0x1000;
    uint32_t tmp_rxlen;
    int32_t err = 0;
    struct x2_qspi_regs *regs = priv->regs;

    level = readl(&regs->rx_fifo_trig_lvl);
    qspi_reset_fifo(&regs->spi_ctl3);

    if (priv->xfer_mode == X2_QSPI_XFER_BATCH) {
        qspi_batch_mode_set(&regs->spi_ctl3, 1);
        do {
            rx_len = rx_remain < 0xFFFF ? rx_remain : 0xFFFF;
            rx_remain -= rx_len;

            /* clear BATCH_RXDONE bit */
            qspi_batch_tx_rx_flag_clr(&regs->status1, 0);

            writel(rx_len, &regs->batch_cnt_rx);

            qspi_enable_rx(&regs->spi_ctl1);

            while (rx_len > 0) {
                if (rx_len > level) {
                    tmp_rxlen = level;
                    if (x2_qspi_poll_rx_empty(&regs->status2, RXFIFO_EMPTY, timeout)) {
                        printf("%s:%d timeout no data fill into rx fifo\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }
                    for (i = 0; i < tmp_rxlen; i++) {
                        ptr[i] = readl(&regs->tx_rx_dat);
                    }

                    rx_len -= tmp_rxlen;
                    ptr += tmp_rxlen;
                } else {
                    tmp_rxlen = rx_len;
                    if (x2_qspi_poll_rx_empty(&regs->status2, RXFIFO_EMPTY, timeout)) {
                        printf("%s:%d timeout no data fill into rx fifo\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }
                    for (i = 0; i < tmp_rxlen; i++) {
                        ptr[i] = readl(&regs->tx_rx_dat);
                    }

                    rx_len -= tmp_rxlen;
                    ptr += tmp_rxlen;
                }
            }
            if (qspi_check_status(&regs->status1, BATCH_RXDONE, timeout)) {
                printf("%s:%d timeout loop batch rx done\n", __func__, __LINE__);
                goto SPI_ERROR;
            }

            qspi_disable_rx(&regs->spi_ctl1);
            /* clear BATCH_RXDONE bit */
            qspi_batch_tx_rx_flag_clr(&regs->status1, 0);
        } while (rx_remain != 0);
    } else {
        qspi_batch_mode_set(&regs->spi_ctl3, 0);

        do {
            rx_len = rx_remain < 0xFFFF ? rx_remain : 0xFFFF;
            rx_remain -= rx_len;
            qspi_enable_rx(&regs->spi_ctl1);

            while (rx_len > 0) {
                if (rx_len > level) {
                    tmp_rxlen = level;
                    if (qspi_check_status(&regs->status2, TXFIFO_EMPTY, timeout)) {
                        printf("%s:%d generate read sclk failed\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }

                    for (i = 0; i < tmp_rxlen; i++)
                        writel(0x0, &regs->tx_rx_dat);

                    if (x2_qspi_poll_rx_empty(&regs->status2, RXFIFO_EMPTY, timeout)) {
                        printf("%s:%d timeout no data fill into rx fifo\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }
                    for (i = 0; i < tmp_rxlen; i++) {
                        ptr[i] = readl(&regs->tx_rx_dat);
                    }

                    rx_len -= tmp_rxlen;
                    ptr += tmp_rxlen;
                } else {
                    tmp_rxlen = rx_len;
                    if (qspi_check_status(&regs->status2, TXFIFO_EMPTY, timeout)) {
                        printf("%s:%d generate read sclk failed\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }

                    for (i = 0; i < tmp_rxlen; i++)
                        writel(0x0, &regs->tx_rx_dat);

                    if (x2_qspi_poll_rx_empty(&regs->status2, RXFIFO_EMPTY, timeout)) {
                        printf("%s:%d timeout no data fill into rx fifo\n", __func__, __LINE__);
                        goto SPI_ERROR;
                    }
                    for (i = 0; i < tmp_rxlen; i++) {
                        ptr[i] = readl(&regs->tx_rx_dat);
                    }

                    rx_len -= tmp_rxlen;
                    ptr += tmp_rxlen;
                }
            }
            qspi_disable_rx(&regs->spi_ctl1);

        } while (rx_remain != 0);
    }
    qspi_reset_fifo(&regs->spi_ctl3);
    return 0;

SPI_ERROR:
    qspi_disable_rx(&regs->spi_ctl1);
    qspi_reset_fifo(&regs->spi_ctl3);
    printf("error: spi rx = %x\n", err);
    return err;
}

int x2_qspi_xfer(struct udevice *dev, unsigned int bitlen, const void *dout,
                 void *din, unsigned long flags)
{
    int ret = 0;
    unsigned int len, cs;
    struct udevice *bus       = dev->parent;
    struct x2_qspi_priv *priv = dev_get_priv(bus);
    struct x2_qspi_regs *regs = priv->regs;
    struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

    if (bitlen == 0) {
        return 0;
    }
    len = bitlen / 8;
    cs  = slave_plat->cs;

    if (flags & SPI_XFER_BEGIN)
        writel(1 << cs, &regs->cs); /* Assert CS before transfer */

    if (dout) {
        ret = x2_qspi_write(priv, dout, len);
    }
    else if (din) {
        ret = x2_qspi_read(priv, din, len);
    }

    if (flags & SPI_XFER_END) {
        writel(0, &regs->cs); /* Deassert CS after transfer */
    }

    if (flags & SPI_XFER_CMD) {
        switch (((u8 *)dout)[0]) {
        case CMD_READ_QUAD_OUTPUT_FAST:
        case CMD_QUAD_PAGE_PROGRAM:
        case CMD_READ_DUAL_OUTPUT_FAST:
            x2_qspi_set_wire(regs, slave_plat->mode);
            break;
        }
    } else {
        x2_qspi_set_wire(regs, 0);
    }

    return ret;
}


static const struct dm_spi_ops x2_qspi_ops = {
    .claim_bus   = x2_qspi_claim_bus,
    .release_bus = x2_qspi_release_bus,
    .xfer        = x2_qspi_xfer,
    .set_speed   = x2_qspi_set_speed,
    .set_mode    = x2_qspi_set_mode,
};

static const struct udevice_id x2_qspi_ids[] = {
    { .compatible = "x2,qspi" },
    { }
};

U_BOOT_DRIVER(qspi) = {
    .name     = "x2_qspi",
    .id       = UCLASS_SPI,
    .of_match = x2_qspi_ids,
    .ops      = &x2_qspi_ops,
    .ofdata_to_platdata       = x2_qspi_ofdata_to_platdata,
    .platdata_auto_alloc_size = sizeof(struct x2_qspi_platdata),
    .priv_auto_alloc_size     = sizeof(struct x2_qspi_priv),
    .probe           = x2_qspi_probe,
    .child_pre_probe = x2_qspi_child_pre_probe,
};
