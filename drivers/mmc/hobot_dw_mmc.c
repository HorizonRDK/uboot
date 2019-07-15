/*
 * Copyright (c) 2013 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dt-structs.h>
#include <dwmmc.h>
#include <errno.h>
#include <mapmem.h>
#include <pwrseq.h>
#include <syscon.h>
#include <linux/err.h>
#include <asm/arch/x2_mmc.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define SDIO0_EMMC_1ST_DIV_CLOCK_HZ 500000000
#define SDIO0_EMMC_2ND_DIV_CLOCK_HZ 62500000

#define SDIO1_EMMC_1ST_DIV_CLOCK_HZ 400000000
#define SDIO1_EMMC_2ND_DIV_CLOCK_HZ 22500000

struct hobot_mmc_plat {
#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_hobot_x2_dw_mshc dtplat;
#endif
	struct mmc_config cfg;
	struct mmc mmc;
};

struct hobot_dwmmc_priv {
	struct clk div1_clk;
	struct clk div2_clk;
	struct clk clk;
	struct dwmci_host host;
	int fifo_depth;
	bool fifo_mode;
	u32 minmax[2];
};
static void sdio0_pin_mux_config(void)
{
    unsigned int reg_val;

    /* GPIO46-GPIO49 GPIO50-GPIO58 */
    reg_val = readl(GPIO3_CFG);
    reg_val &= 0xFC000000;
    writel(reg_val, GPIO3_CFG);

	/* GPIO78-GPIO83 */
    reg_val = readl(GPIO5_CFG);
    reg_val &= 0xFFFFF000;
    writel(reg_val, GPIO5_CFG);

	/* For x2 dev 3.3v voltage*/
    reg_val = readl(GPIO7_CFG);
    reg_val |= 0x00003000;
    writel(reg_val, GPIO7_CFG);

    reg_val = readl(GPIO7_DIR);
    reg_val |= 0x00400000;
    reg_val &= 0xffffffbf;
    writel(reg_val, GPIO7_DIR);

	/* For j2 dev 3.3v voltage*/
    reg_val = readl(GPIO5_CFG);
    reg_val |= 0x03000000;
    writel(reg_val, GPIO5_CFG);

    reg_val = readl(GPIO5_DIR);
    reg_val |= 0x10000000;
    reg_val &= 0xffffefff;
    writel(reg_val, GPIO5_DIR);
}
static uint hobot_dwmmc_get_mmc_clk(struct dwmci_host *host, uint freq)
{
#ifndef CONFIG_TARGET_X2_FPGA
	struct udevice *dev = host->priv;
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);

	freq = clk_get_rate(&priv->clk);

#else
	freq = 50000000;
#endif

	return freq;
}

static int hobot_dwmmc_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	host->buswidth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
					"bus-width", 4);
	host->get_mmc_clk = hobot_dwmmc_get_mmc_clk;
	host->priv = dev;

	/* use non-removeable as sdcard and emmc as judgement */
	if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev), "non-removable"))
		host->dev_index = 0;
	else
		host->dev_index = 1;

	priv->fifo_depth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
				    "fifo-depth", 0);
	if (priv->fifo_depth < 0)
		return -EINVAL;
	priv->fifo_mode = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
                                     "fifo-mode",0);
	if (fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev),
				 "clock-freq-min-max", priv->minmax, 2))
		return -EINVAL;
#endif
	return 0;
}

static int hobot_dwmmc_probe(struct udevice *dev)
{
	struct hobot_mmc_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;
	struct udevice *pwr_dev __maybe_unused;
	unsigned long clock;
	int ret;

#if CONFIG_IS_ENABLED(OF_PLATDATA)

	struct dtd_hobot_x2_dw_mshc *dtplat = &plat->dtplat;

	host->name = dev->name;
	host->ioaddr = map_sysmem(dtplat->reg[0], dtplat->reg[1]);
	host->buswidth = dtplat->bus_width;
	host->get_mmc_clk = hobot_dwmmc_get_mmc_clk;
	host->priv = dev;
	host->dev_index = 0;
	priv->fifo_depth = dtplat->fifo_depth;
	priv->fifo_mode = 0;
	memcpy(priv->minmax, dtplat->clock_freq_min_max, sizeof(priv->minmax));

	ret = clk_get_by_index_platdata(dev, 0, dtplat->clocks, &priv->clk);
	if (ret < 0)
		return ret;

#else
#ifndef CONFIG_TARGET_X2_FPGA

	ret = clk_get_by_index(dev, 0, &priv->div1_clk);
	if (ret < 0) {
		debug("failed to get 1st div clk.\n");
		return ret;
	}

	if (host->dev_index == 0) {
		ret = clk_set_rate(&priv->div1_clk, SDIO0_EMMC_1ST_DIV_CLOCK_HZ);
		if(ret < 0){
			debug("failed to set 1st div rate.\n");
			return ret;
		}
	} else {
		ret = clk_set_rate(&priv->div1_clk, SDIO1_EMMC_1ST_DIV_CLOCK_HZ);
		if(ret < 0){
			debug("failed to set 1st div rate.\n");
			return ret;
		}
	}

	ret = clk_get_by_index(dev, 1, &priv->div2_clk);
	if (ret < 0) {
		debug("failed to get 2nd div clk.\n");
		return ret;
	}

	if (host->dev_index == 0) {
		ret = clk_set_rate(&priv->div2_clk, SDIO0_EMMC_2ND_DIV_CLOCK_HZ);
		if(ret < 0){
			debug("failed to set 2nd div rate.\n");
			return ret;
		}
	} else {
		ret = clk_set_rate(&priv->div2_clk, SDIO1_EMMC_2ND_DIV_CLOCK_HZ);
		if(ret < 0){
			debug("failed to set 2nd div rate.\n");
			return ret;
		}
	}

	ret = clk_get_by_index(dev, 2, &priv->clk);
	if (ret < 0) {
		debug("failed to get gate clk.\n");
		return ret;
	}

	clock = clk_get_rate(&priv->clk);
	if(IS_ERR_VALUE(clock)) {
		debug("failed to get clk rate.\n");
		return clock;
	}

	debug("emmc host clock set %ld.\n", clock);
	clk_enable(&priv->clk);

#endif
#endif
	host->fifoth_val = MSIZE(0x2) |
		RX_WMARK(priv->fifo_depth / 2 - 1) |
		TX_WMARK(priv->fifo_depth / 2);

	host->fifo_mode = priv->fifo_mode;

#ifdef CONFIG_PWRSEQ
	/* Enable power if needed */
	ret = uclass_get_device_by_phandle(UCLASS_PWRSEQ, dev, "mmc-pwrseq",
					   &pwr_dev);
	if (!ret) {
		ret = pwrseq_set_power(pwr_dev, true);
		if (ret)
			return ret;
	}
#endif
	dwmci_setup_cfg(&plat->cfg, host, priv->minmax[1], priv->minmax[0]);
	host->mmc = &plat->mmc;
	host->mmc->priv = &priv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

    sdio0_pin_mux_config();

    ret = dwmci_probe(dev);
	if (ret) {
		printf("dwmci_probe fail\n");
	}
    ret = mmc_init(host->mmc);
    if (ret)
    {
        printf("mmc_init fail\n");
    }
	mmc_set_rst_n_function(host->mmc, 0x01);
    return ret;
}

static int hobot_dwmmc_bind(struct udevice *dev)
{
	struct hobot_mmc_plat *plat = dev_get_platdata(dev);
	return dwmci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id hobot_dwmmc_ids[] = {
	{ .compatible = "hobot,x2-dw-mshc" },
	{ }
};
U_BOOT_DRIVER(hobot_dwmmc_drv) = {
    .name		= "hobot_dwmmc",
    .id		    = UCLASS_MMC,
    .of_match	= hobot_dwmmc_ids,
    .ofdata_to_platdata = hobot_dwmmc_ofdata_to_platdata,
    .ops		= &dm_dwmci_ops,
    .bind		= hobot_dwmmc_bind,
    .probe		= hobot_dwmmc_probe,
    .priv_auto_alloc_size = sizeof(struct hobot_dwmmc_priv),
    .platdata_auto_alloc_size = sizeof(struct hobot_mmc_plat),
    .flags = DM_FLAG_PRE_RELOC,
};
#ifdef CONFIG_PWRSEQ
static int hobot_dwmmc_pwrseq_set_power(struct udevice *dev, bool enable)
{
	struct gpio_desc reset;
	int ret;

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &reset, GPIOD_IS_OUT);
	if (ret)
		return ret;
	dm_gpio_set_value(&reset, 1);
	udelay(1);
	dm_gpio_set_value(&reset, 0);
	udelay(200);

	return 0;
}

static const struct pwrseq_ops hobot_dwmmc_pwrseq_ops = {
	.set_power	= hobot_dwmmc_pwrseq_set_power,
};

static const struct udevice_id hobot_dwmmc_pwrseq_ids[] = {
	{ .compatible = "mmc-pwrseq-emmc" },
	{ }
};

U_BOOT_DRIVER(hobot_dwmmc_pwrseq_drv) = {
	.name		= "mmc_pwrseq_emmc",
	.id		= UCLASS_PWRSEQ,
	.of_match	= hobot_dwmmc_pwrseq_ids,
	.ops		= &hobot_dwmmc_pwrseq_ops,
};
#endif
