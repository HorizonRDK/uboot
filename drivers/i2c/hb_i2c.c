/*
* Copyright [2019] Horizon
*/

#include <asm/io.h>
#include <common.h>
#include <errno.h>
#include <dm.h>
#include <fdtdec.h>
#include <i2c.h>
#include "hb_i2c.h"

DECLARE_GLOBAL_DATA_PTR;
#define X2_I2C_FIFO_SIZE	16
#define WAIT_IDLE_TIMEOUT	200
#define I2C_TIMEOUT_MS		100

#define PERISYS_CLKEN	0xA1000154
#define X2_RESET_CTL	0xA1000450
#ifdef CONFIG_TARGET_XJ3
#define X2_RESET_CTL_I2C	10
#else
#define X2_RESET_CTL_I2C	11
#endif

#define X2_PIN_CTL_0	0xA6003000
#define X2_PIN_CTL_4	0xA6003040
#define X3_PIN_SW		0xA6004000

static int hb_i2c_wait_idle(struct hb_i2c_bus *priv)
{
	int timeout = WAIT_IDLE_TIMEOUT;
	union status_reg_e status;

	status.all = readl(&priv->regs->status);
	//printk("%s, status:0x%x\n", __func__, status.all);
	while(status.bit.busy) {
		if (timeout < 0) {
			printf("%s, timeout waiting for bus ready, and status:0x%x\n",
             __func__, status.all);
			return -ETIMEDOUT;
		}

		status.all = readl(&priv->regs->status);
		timeout--;
	}

	return 0;
}
static int hb_i2c_cfg(struct hb_i2c_bus *dev, int dir_rd, int timeout_enable)
{
	union cfg_reg_e cfg;
	cfg.all = 0;
	cfg.bit.tran_en = 1;
	cfg.bit.en = 1;

	if (timeout_enable) {
		cfg.bit.to_en = 1;
	} else {
		cfg.bit.to_en = 0;
	}

	cfg.bit.dir_rd = dir_rd;
	cfg.bit.clkdiv = dev->clk_div;
	//cfg.bit.clkdiv = 0xef;
	writel(0xffff, &dev->regs->tocnt);
	writel(cfg.all, &dev->regs->cfg);
	//printk("%s,and cfg:0x%x\n", __func__, readl(&dev->regs->cfg));
	return 0;
}
int hb_i2c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	struct hb_i2c_bus *priv = dev_get_priv(bus);

	int div;

	if (speed < 40000 || speed > 400000) {
		printf("%s, cannot support freq %d\n", __func__, speed);
		return -1;
	}
	priv->clock_freq = speed;
	div =  DIV_ROUND_UP(24000000, priv->clock_freq) - 1;
	priv->clk_div = div;
	hb_i2c_cfg(priv, 0, 0);
	return 0;
}

static void hb_i2c_mask_int(struct hb_i2c_bus *priv)
{
	writel(0xffffffff, &priv->regs->intsetmask);
}

static void hb_i2c_unmask_int(struct hb_i2c_bus *priv)
{
	union intunmask_reg_e unmask_reg;

	unmask_reg.all = 0;
	unmask_reg.bit.tr_done_mask = 1;
	unmask_reg.bit.to_mask = 1;
	unmask_reg.bit.al_mask = 1;
	unmask_reg.bit.sterr_mask = 1;
	unmask_reg.bit.nack_mask = 1;
	unmask_reg.bit.aerr_mask = 1;

	if (priv->op_flags == I2C_READ)
		unmask_reg.bit.rrdy_mask = 1;
	else if (priv->op_flags == I2C_WRITE)
		unmask_reg.bit.xrdy_mask = 1;
	else {
		unmask_reg.bit.xrdy_mask = 1;
		unmask_reg.bit.rrdy_mask = 1;
	}
	writel(unmask_reg.all, &priv->regs->intunmask);
}


static void hb_i2c_clear_int_status(struct hb_i2c_bus *priv)
{
	writel(0xffffffff, &priv->regs->srcpnd);
}

static int hb_i2c_read(struct hb_i2c_bus *priv, struct i2c_msg *msg)
{
	uint bytes_remain_len = msg->len;
	uint bytes_xferred = 0;
	union ctl_reg_e ctl_reg;
	union dcount_reg_e dcount_reg;
	int i;
	int err;
	ulong start;
	union sprcpnd_reg_e int_status;
	union status_reg_e status;


//	printf("%s,and msg->len:%d\n", __func__, msg->len);
	priv->rx_buf = msg->buf;
	priv->rx_remaining = msg->len;

	priv->msg_err = 0;
	if (hb_i2c_wait_idle(priv))
		return -ETIMEDOUT;

        hb_i2c_mask_int(priv);

        ctl_reg.all = 0;
        dcount_reg.all = 0;

        if (msg->flags & I2C_M_TEN) {
                writel(msg->addr << 1| BIT(11), &priv->regs->addr);
        } else {
                writel(msg->addr << 1, &priv->regs->addr);
        }

        if (hb_i2c_wait_idle(priv))
		return -ETIMEDOUT;

	dcount_reg.bit.r_dcount = bytes_remain_len;
	writel(dcount_reg.all, &priv->regs->dcount);

	ctl_reg.bit.rd = 1;
	ctl_reg.bit.sta = 1;
	ctl_reg.bit.sto = 1;
	writel(ctl_reg.all, &priv->regs->ctl);

	while(bytes_remain_len) {
		if (bytes_remain_len > X2_I2C_FIFO_SIZE)
			bytes_xferred = X2_I2C_FIFO_SIZE;
		else
			bytes_xferred = bytes_remain_len;

		hb_i2c_unmask_int(priv);

	    udelay(1000);
    	start = get_timer(0);
		while (1) {
			int_status.all = readl(&priv->regs->srcpnd);
			err = int_status.bit.nack | int_status.bit.sterr | int_status.bit.al|
			int_status.bit.to | int_status.bit.aerr;

			hb_i2c_clear_int_status(priv);
		//	printk("%s, after clear int,and read:0x%x\n",__func__,readl(&priv->regs->srcpnd));
			if (err) {
				writel(int_status.all, &priv->regs->srcpnd);
				err = -EREMOTEIO;
			}
			if (int_status.bit.tr_done || int_status.bit.rrdy) {

				//writel(int_status.all, &priv->regs->srcpnd);

				break;
			}
			if (get_timer(start) > I2C_TIMEOUT_MS) {
				printf("%s, hb i2c read data Timeout\n", __func__);
				err = -ETIMEDOUT;
				goto i2c_exit;
			}

			udelay(1000);
		}
		for (i = 0; i < bytes_xferred; i++) {
			status.all = readl(&priv->regs->status);
			if (status.bit.rx_empty) {
				printf("%s rx empty\n", __func__);
				break;
			}
			*(priv->rx_buf) = readl(&priv->regs->rdata) & 0xff;
		//	printf("%s, and rx_buf[%d]:%d\n", __func__, i, *(priv->rx_buf));
			priv->rx_buf++;
		}

		bytes_remain_len -= i;
	}

i2c_exit:
	ctl_reg.all = 0;
	ctl_reg.bit.rfifo_clr = 1;
	ctl_reg.bit.tfifo_clr = 1;
	writel(ctl_reg.all, &priv->regs->ctl);
	writel(0x0, &priv->regs->ctl);

	return err;
}


static int hb_i2c_write(struct hb_i2c_bus *priv, struct i2c_msg *msg)
{
	uint bytes_remain_len = msg->len;
	uint bytes_xferred = 0;
	int i;
	int err;
	union ctl_reg_e ctl_reg;
	union dcount_reg_e dcount_reg;
	ulong start;
	union sprcpnd_reg_e int_status;
	union status_reg_e status;

	priv->tx_buf = msg->buf;
	priv->tx_remaining = msg->len;

	priv->msg_err = 0;

	if (hb_i2c_wait_idle(priv))
		return -ETIMEDOUT;

//	printf("%s, and len:%d\n", __func__, msg->len);

	hb_i2c_mask_int(priv);
	ctl_reg.all = 0;
	dcount_reg.all = 0;
	if (msg->flags & I2C_M_TEN) {
		writel(msg->addr << 1| BIT(11), &priv->regs->addr);
	} else {
		writel(msg->addr << 1, &priv->regs->addr);
	}
	if (hb_i2c_wait_idle(priv))
		return -ETIMEDOUT;

	//设置count数量
	dcount_reg.bit.w_dcount = bytes_remain_len;
	writel(dcount_reg.all, &priv->regs->dcount);

	while (bytes_remain_len) {
		if (bytes_remain_len > X2_I2C_FIFO_SIZE)
			bytes_xferred = X2_I2C_FIFO_SIZE;
		else
			bytes_xferred = bytes_remain_len;
		//printf("%s,and bytes_xferred:%d\n", __func__, bytes_xferred);
		for (i = 0; i < bytes_xferred; i++) {
			status.all = readl(&priv->regs->status);
			if (status.bit.tx_full) {
				printf("%s tx full\n", __func__);
				break;
			}
		//	printf("%s, tx_buf[%d]:%d\n", __func__, i, *(priv->tx_buf));
			writel(*(priv->tx_buf), &priv->regs->tdata);
			priv->tx_buf++;
		}

		if (bytes_remain_len == msg->len) {
			//设置开始发送.
			ctl_reg.bit.wr = 1;
			ctl_reg.bit.sta = 1;
			ctl_reg.bit.sto = 1;
			writel(ctl_reg.all, &priv->regs->ctl);
		}

		hb_i2c_unmask_int(priv);
		//开始轮寻
        udelay(1000);
		start = get_timer(0);
		while (1) {
			int_status.all = readl(&priv->regs->srcpnd);
			err = int_status.bit.nack | int_status.bit.sterr | int_status.bit.al|
				int_status.bit.to | int_status.bit.aerr;
			hb_i2c_clear_int_status(priv);
			if (err) {
				writel(int_status.all, &priv->regs->srcpnd);
				err = -EREMOTEIO;
			}
			if (int_status.bit.tr_done || int_status.bit.xrdy) {
				writel(int_status.all,&priv->regs->srcpnd);
				break;
			}

			if (get_timer(start) > I2C_TIMEOUT_MS) {
				printf("%s, hb i2c write data timeout\n", __func__);
				err = -ETIMEDOUT;
				goto i2c_exit;
			}
			udelay(1000);
		}

		bytes_remain_len -= i;
	}

i2c_exit:
	//发送停止位
	ctl_reg.all = 0;
	ctl_reg.bit.rfifo_clr = 1;
	ctl_reg.bit.tfifo_clr = 1;
	writel(ctl_reg.all, &priv->regs->ctl);
	writel(0x0, &priv->regs->ctl);

	return err;
}
static int hb_i2c_xfer(struct udevice *bus, struct i2c_msg *msg, int nmsgs)
{
	struct hb_i2c_bus *priv = dev_get_priv(bus);
	int ret = 0;
	int i;

	if (msg[0].flags & 0x20) {
		hb_i2c_cfg(priv, 0, 0);
	} else {
		hb_i2c_cfg(priv, 1, 1);
	}

//	printk("%s, nmsgs:%d, flags:0x%x\n", __func__, nmsgs,flags);
	for (i = 0; i < nmsgs; i++) {
//		printk("%s, this %d msg, and i2c_m_rd:0x%0x, and msg_flags:0x%x\n", __func__, i,I2C_M_RD, msg[i].flags);
		if (msg[i].flags & I2C_M_RD)
			ret = hb_i2c_read(priv, &msg[i]);
		else
			ret = hb_i2c_write(priv, &msg[i]);


		if (ret) {
			printf("%s, error...\n", __func__);
			return -EREMOTEIO;
		}
	}


	return 0;
}

#if 1
static int hb_i2c_transfer(struct hb_i2c_bus *priv, uchar chip, uchar data[], uint data_len)
{
	int err = 0;
	ulong start = get_timer(0);
	union ctl_reg_e ctl_reg;
    union dcount_reg_e dcount_reg;
    union sprcpnd_reg_e int_status;
    union status_reg_e status;
	//union cfg_reg_e cfg;
	int command = 0;

	hb_i2c_mask_int(priv);

	hb_i2c_clear_int_status(priv);
	//配置产生ack和nack就可以了.
    ctl_reg.all = 0;
    dcount_reg.all = 0;
	//配置地址

#if 1
	priv->tx_buf = (u8 *)&command;
	priv->tx_remaining = 1;
	priv->rx_remaining = 1;
	dcount_reg.bit.w_dcount = 1;
	writel(dcount_reg.all, &priv->regs->dcount);

	//printf("%s,and write count:%d\n", __func__, readl(&priv->regs->dcount));
    writel(*(priv->tx_buf), &priv->regs->tdata);
    //printf("%s,and tdata:%d\n", __func__, readl(&priv->regs->tdata));
    int_status.all = readl(&priv->regs->srcpnd);
    //	printf("%s, after write tx buf, status:0x%x, int_status:0x%x\n", __func__,readl(&priv->regs->status), int_status.all);

    //	if (x2_i2c_wait_idle(priv))
    ///		return -ETIMEDOUT;

#endif

    priv->op_flags = I2C_READ;
    priv->rx_buf = data;
    dcount_reg.bit.r_dcount = 1;// data_len;
    writel(dcount_reg.all, &priv->regs->dcount);
    //write data

    //	printf("%s,and dcount_reg.all: 0x%x,  r_dcount:0x%x\n", __func__, dcount_reg.all,readl(&priv->regs->dcount));
    priv->rx_remaining = 1;//data_len;
    ctl_reg.bit.rd = 1;

	writel(chip << 1, &priv->regs->addr);
         ctl_reg.bit.sta = 1;
                ctl_reg.bit.sto = 1;
//	hb_i2c_clear_int_status(priv);
	hb_i2c_unmask_int(priv);


//	printf("%s, before ctl:0x%x\n", __func__, readl(&priv->regs->ctl));
//	printf("%s, cfg:0x%x, intmask:0x%x, intsetmask:0x%x, intunmask:0x%x\n", __func__,readl(&priv->regs->cfg), readl(&priv->regs->intmask), readl(&priv->regs->intsetmask), readl(&priv->regs->intunmask));
	 writel(ctl_reg.all, &priv->regs->ctl);

//	printf("%s, and ctl:0x%x\n", __func__, readl(&priv->regs->ctl));

	int_status.all = readl(&priv->regs->srcpnd);
//	printf("%s, before while, sts:0x%x, int_status:0x%x\n", __func__,readl(&priv->regs->status), int_status.all);
	start = get_timer(0);
                while (1) {

			status.all = readl(&priv->regs->status);
                        int_status.all = readl(&priv->regs->srcpnd);
                        err = int_status.bit.nack | int_status.bit.sterr | int_status.bit.al|
				int_status.bit.to| int_status.bit.aerr;

		//	printk("%s, after while: status:0x%x, int_status:0x%x\n", __func__, status.all, int_status.all);

			hb_i2c_clear_int_status(priv);
		//	printf("%s, after clear int srcpnd:0x%x\n", __func__, readl(&priv->regs->srcpnd));
		//	printk("%s, status:0x%x\n", __func__, status.all);

			if (err) {

				err = -EIO;
				goto i2c_exit;
			} else {

				if (int_status.bit.tr_done || int_status.bit.rrdy || int_status.bit.xrdy) {
                if (int_status.bit.tr_done) {
					while(priv->rx_remaining) {
						status.all = readl(&priv->regs->status);
						if (status.bit.rx_empty) {
							printf("%s rx empty, and status:0x%x\n",__func__, status.all);
							break;
						}
						*(priv->rx_buf) = readl(&priv->regs->rdata) & 0xff;
						priv->rx_remaining--;
						priv->rx_buf++;
					}
						//*(priv->rx_buf) = readl(&priv->regs->rdata) & 0xff;
						err = 0;
						break;
					} else {
						writel(0x2, &priv->regs->fifo_ctl);

					while(priv->rx_remaining) {
						status.all = readl(&priv->regs->status);
						if (status.bit.rx_empty) {
							printf("%s rx empty, and status:0x%x\n",__func__, status.all);
							break;
						}
						*(priv->rx_buf) = readl(&priv->regs->rdata) & 0xff;
						priv->rx_remaining--;
						priv->rx_buf++;
					}

						//*(priv->rx_buf) = readl(&priv->regs->rdata) & 0xff;
						writel(0x0, &priv->regs->fifo_ctl);
						err = 0;
						break;
					}
				}
			}

                        if (get_timer(start) > I2C_TIMEOUT_MS) {
                                printf("%s, hb i2c read data Timeout\n", __func__);
                                err = -ETIMEDOUT;
				goto i2c_exit;
                        }

                        udelay(1000);
                }

i2c_exit:
        //发送停止位



        hb_i2c_mask_int(priv);
        ctl_reg.all = 0;
        ctl_reg.bit.rfifo_clr = 1;
        ctl_reg.bit.tfifo_clr = 1;
        writel(ctl_reg.all, &priv->regs->ctl);

        return err;


}

static void hb_i2c_reset(struct hb_i2c_bus *priv)
{
	uint32_t value = 0;

	value = readl(priv->hb_reset);
	value |= 1 << (priv->bus_num + X2_RESET_CTL_I2C);
	writel(value, priv->hb_reset);


	udelay(1000);

	value = readl(priv->hb_reset);
	value &= ~(1 << (priv->bus_num + X2_RESET_CTL_I2C));
	writel(value, priv->hb_reset);
}
static int hb_i2c_probe_chip(struct udevice *bus, uint chip, uint chip_flags)
{
	struct hb_i2c_bus *priv = dev_get_priv(bus);
	uchar buf[1];
	int ret;

	//printf("%s,and here\n", __func__);
	buf[0] = 0;

	hb_i2c_reset(priv);
	hb_i2c_cfg(priv, 0, 1);

	ret = hb_i2c_transfer(priv, chip, buf, 0);


	return ret;
}

#endif

static const struct dm_i2c_ops hb_i2c_ops = {
	.xfer = hb_i2c_xfer,
	.set_bus_speed = hb_i2c_set_bus_speed,
	.probe_chip = hb_i2c_probe_chip,
};


static int hb_i2c_ofdata_to_platdata(struct udevice *dev)
{
	const void *blob = gd->fdt_blob;
	struct hb_i2c_bus *priv = dev_get_priv(dev);
	int node;
	int value;
	int bus_nr, clken_bit;
	int ret = 0;

	node = dev_of_offset(dev);
	priv->regs = (struct hb_i2c_regs_s *)devfdt_get_addr(dev);

	ret = dev_read_alias_seq(dev, &bus_nr);
	if (ret < 0) {
		printf("%s,Could not get alias for %s:%d\n", __func__, dev->name, ret);
		return ret;
	}

	priv->bus_num = bus_nr;

#ifdef CONFIG_TARGET_XJ3
	switch (priv->bus_num) {
	case 0:
		priv->pin_first = 0x20;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 0;
		clken_bit = 11;
		break;
	case 1:
		priv->pin_first = 0x28;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 0;
		clken_bit = 12;
		break;

	case 2:
		priv->pin_first = 0x30;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 0;
		clken_bit = 13;
		break;

	case 3:
		priv->pin_first = 0x38;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 0;
		clken_bit = 14;
		break;

	case 4:
		priv->pin_first = 0x40;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 1;
		clken_bit = 23;
		break;

	case 5:
		priv->pin_first = 0x48;
		priv->pin_ctl = (void __iomem *)(X3_PIN_SW);
		priv->i2c_func = 1;
		clken_bit = 24;
		break;

	default:
		printf("%s:bus_num:%d error\n", __func__, priv->bus_num);
		return -1;
	}

	value = readl(priv->pin_ctl + priv->pin_first);
	value &= ~0x3;
	value |= priv->i2c_func;
	writel(value, priv->pin_ctl + priv->pin_first);
	value = readl(priv->pin_ctl + priv->pin_first + 4);
	value &= ~0x3;
	value |= priv->i2c_func;
	writel(value, priv->pin_ctl + priv->pin_first + 4);
#else
	switch (priv->bus_num) {
	case 0:
		priv->pin_first = 9;
		priv->pin_ctl = (void __iomem *)(X2_PIN_CTL_0);
		priv->i2c_func = 0;
		clken_bit = 11;
		break;
	case 1:
		priv->pin_first = 11;
		priv->pin_ctl = (void __iomem *)(X2_PIN_CTL_0);
		priv->i2c_func = 1;
		clken_bit = 12;
		break;

	case 2:
		priv->pin_first = 14;
		priv->pin_ctl = (void __iomem *)(X2_PIN_CTL_4);
		priv->i2c_func = 1;
		clken_bit = 13;
		break;

	case 3:
		priv->pin_first =  4;
		priv->pin_ctl = (void __iomem *)(X2_PIN_CTL_4);
		priv->i2c_func = 2;
		clken_bit = 14;
		break;

	default:
		printf("%s:bus_num:%d error\n", __func__, priv->bus_num);
		return -1;
	}

	value = readl(priv->pin_ctl);
	value &= ~(0xf << (priv->pin_first * 2);
	value |= (priv->i2c_func << (priv->pin_first * 2));
	value |= (priv->i2c_func << (priv->pin_first * 2 + 2));
	writel(value, priv->pin_ctl);
#endif

	priv->perisys_clk = (void __iomem *)(PERISYS_CLKEN);
	value = readl(priv->perisys_clk);
	value |= (1 << clken_bit);
	writel(value, priv->perisys_clk);

	priv->hb_reset = (void __iomem *)(X2_RESET_CTL);

	priv->clock_freq = fdtdec_get_int(blob, node, "clock-frequency", 100000);

//	printf("%s, and freq:%d\n", __func__, priv->clock_freq);
	if (priv->clock_freq < 40000 || priv->clock_freq > 400000) {
		printf("%s, Cannot supported frequency, so set default\n", __func__);
		priv->clock_freq = 100000;
	}
	priv->clk_div = DIV_ROUND_UP(24000000, priv->clock_freq) - 1;
	return 0;

}


static const struct udevice_id hb_i2c_ids[] = {
	{.compatible = "hobot,hb-i2c"},
	{}
};
U_BOOT_DRIVER(i2c_hb) = {
	.name = "i2c_hb",
	.id = UCLASS_I2C,
	.of_match = hb_i2c_ids,
	.ofdata_to_platdata = hb_i2c_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct hb_i2c_bus),
	.ops = &hb_i2c_ops,

};
