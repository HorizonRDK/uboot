/*
 * Copyright (c) 2013 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/of_access.h>
#include <dt-structs.h>
#include <dwmmc.h>
#include <errno.h>
#include <mapmem.h>
#include <pwrseq.h>
#include <syscon.h>
#include <linux/err.h>
#include <asm/arch/hb_mmc.h>
#include <mmc.h>
#include <hb_info.h>

DECLARE_GLOBAL_DATA_PTR;

#define HOBOT_SYSCTRL_REG		(0xA1000000)
#define HOBOT_MMC_CLK_REG(ctrl_id) \
				(u64)(HOBOT_SYSCTRL_REG + 0x320 + ctrl_id * 0x10)

#define DWMMC_MMC_ID			(0)
#define DWMMC_SD1_ID			(1)
#define DWMMC_SD2_ID			(2)
#define HO
#define HOBOT_DW_MCI_FREQ_MAX		(200000000)

#define HOBOT_MMC_CLK_DIS		(HOBOT_SYSCTRL_REG + 0x158)
#define HOBOT_MMC_CLK_EN			(HOBOT_SYSCTRL_REG + 0x154)
#define HOBOT_SD0_CLK_SHIFT	BIT(15)
#define HOBOT_SD1_CLK_SHIFT	BIT(16)
#define HOBOT_SD2_CLK_SHIFT	BIT(25)

#define HOBOT_CLKOFF_STA		(0x258)
#define HOBOT_SD0_CLKOFF_STA_SHIFT	(1<<1)
#define HOBOT_SD1_CLKOFF_STA_SHIFT	(1<<2)
#define HOBOT_SD2_CLKOFF_STA_SHIFT	(1<<3)

/*#define HOBOT_MMC_DEGREE_MASK			(0xF)*/
#define HOBOT_MMC_SAMPLE_DEGREE_SHIFT	(12)
#define HOBOT_MMC_DRV_DEGREE_SHIFT	(8)
#define SDCARD_RD_THRESHOLD		(512)
#define NUM_PHASES			(16)
#define TUNING_ITERATION_TO_PHASE(i)	(i)

struct hobot_mmc_plat {
#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_hobot_hb_dw_mshc dtplat;
#endif
	struct mmc_config cfg;
	struct mmc mmc;
};

struct hobot_dwmmc_priv {
	struct clk clk;
	struct dwmci_host host;
	int ctrl_id;
	int fifo_depth;
	bool fifo_mode;
	unsigned int current_sample_phase;
	unsigned int default_sample_phase;
	u32 minmax[2];
};

static void sdio_reset(int dev_index)
{
#ifdef CONFIG_TARGET_X3
	u32 val = 0;
	u32 rst_val = 0;

	if (dev_index == 0)
		rst_val = SD0_RSTN;
	else if (dev_index == 1)
		rst_val = SD1_RSTN;
	else if (dev_index == 2)
		rst_val = SD2_RSTN;
	else
		return;

//	val = readl(SYSCTRL_BASE + 0x450);
//	val |= rst_val;
//	writel(val, SYSCTRL_BASE + 0x450);
//	udelay(100);
	val &= (~rst_val);
	writel(val, SYSCTRL_BASE + 0x450);
#endif
}

static void sdio_power(int dev_index)
{
#ifdef CONFIG_TARGET_X3
	u32 val = 0;
        uint32_t base_board_id = hb_base_board_type_get();

        switch (base_board_id) {
        case BASE_BOARD_X3_DVB:
                debug("%s base board type: X3 DVB\n", __func__);
                break;
        case BASE_BOARD_J3_DVB:
                debug("%s base board type: J3 DVB\n", __func__);
		if (dev_index == 1) {
			val = readl(SD1_POWER_PIN_MUX);
			val |= 0x03;
			writel(val, SD1_POWER_PIN_MUX);

			val = readl(SD1_POWER_OUTPUT_CTRL);
			val |= SD1_POWER_DIR;
			val &= ~(SD1_POWER_OUTPUT);
			writel(val, SD1_POWER_OUTPUT_CTRL);
		}
                break;
        case BASE_BOARD_CVB:
                debug("%s base board type: CVB\n", __func__);
                break;
        case BASE_BOARD_X3_SDB:
                debug("%s base board type: X3 SDB\n", __func__);
                break;
        default:
                debug("%s base board type not support\n", __func__);
                break;
        }
#endif
}

static void sdio_pin_mux_config(int dev_index)
{
	unsigned int reg_val;

#ifdef CONFIG_TARGET_X2
	struct hb_info_hdr* boot_info = (struct hb_info_hdr*) HB_BOOTINFO_ADDR;

	if (dev_index != 0)
		return;
	/* GPIO46-GPIO49 GPIO50-GPIO58 */
	reg_val = readl(GPIO3_CFG);
	reg_val &= 0xFC000000;
	writel(reg_val, GPIO3_CFG);

	/* GPIO78-GPIO83 */
	reg_val = readl(GPIO5_CFG);
	reg_val &= 0xFFFFF000;
	writel(reg_val, GPIO5_CFG);
#if 0
	if (boot_info->board_id == HB_DEV_BOARD_ID) {
		/* For x2 dev 3.3v voltage*/
		reg_val = readl(GPIO7_CFG);
		reg_val |= 0x00003000;
		writel(reg_val, GPIO7_CFG);

		reg_val = readl(GPIO7_DIR);
		reg_val |= 0x00400000;
		reg_val &= 0xffffffbf;
		writel(reg_val, GPIO7_DIR);
	}

	if (boot_info->board_id == J2_SOM_DEV_ID) {
		/* For j2 dev 3.3v voltage*/
		reg_val = readl(GPIO5_CFG);
		reg_val |= 0x03000000;
		writel(reg_val, GPIO5_CFG);

		reg_val = readl(GPIO5_DIR);
		reg_val |= 0x10000000;
		reg_val &= 0xffffefff;
		writel(reg_val, GPIO5_DIR);
	}
#endif
#endif
#ifdef CONFIG_TARGET_X3
	u64 i;
	if (dev_index == 0) {
		for (i = SD0_CLK; i <= SD0_WPROT; i += 4) {
			reg_val = readl(i);
			reg_val &= 0xFFFFFFFC;
			writel(reg_val, i);
		}
	} else	if (dev_index == 1) {
		for (i = SD1_CLK; i <= SD1_DATA3; i += 4) {
			reg_val = readl(i);
			reg_val &= 0xFFFFFFFC;
			writel(reg_val, i);
		}
	} else if (dev_index == 2) {
		for (i = SD2_CLK; i <= SD2_DATA3; i += 4) {
			reg_val = readl(i);
			reg_val &= 0xFFFFFFFC;
			writel(reg_val, i);
		}
	}
#endif
}

static uint hobot_dwmmc_get_mmc_clk(struct dwmci_host *host, uint freq)
{
#if !defined(CONFIG_TARGET_X2_FPGA) && !defined(CONFIG_TARGET_X3_FPGA)
	struct udevice *dev = host->priv;
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);
	unsigned int reg_val = 0, mmc_shift;
	int tmp = 0;

	/* decide which ctrl we are configuring */
	mmc_shift = (priv->ctrl_id == 0 ? HOBOT_SD0_CLK_SHIFT :
			   (priv->ctrl_id == 1 ? HOBOT_SD1_CLK_SHIFT :
									 HOBOT_SD2_CLK_SHIFT));
	/* Disable clk */
	writel(mmc_shift, HOBOT_MMC_CLK_DIS);
	udelay(500);
	/* Configure 1st div to 8 */
	reg_val = readl(HOBOT_MMC_CLK_REG(priv->ctrl_id));
	reg_val &= 0xFFFFFF00;
	reg_val |= 0x70;
	writel(reg_val, HOBOT_MMC_CLK_REG(priv->ctrl_id));
	/* Configure 2nd div */
	tmp = clk_get_rate(&priv->clk) / freq;
	reg_val = readl(HOBOT_MMC_CLK_REG(priv->ctrl_id));
	reg_val |= (tmp & 0xF);
	writel(reg_val, HOBOT_MMC_CLK_REG(priv->ctrl_id));
	/* Enable clk */
	writel(mmc_shift, HOBOT_MMC_CLK_EN);
	udelay(500);
	clk_set_rate(&priv->clk, freq);
	freq = clk_get_rate(&priv->clk);
#else
	freq = 50000000;
#endif
	pr_debug("%s: actual freq: %u\n", host->name, freq);
	return freq;
}

#ifdef MMC_SUPPORTS_TUNING
static int hb_mmc_set_sample_phase(struct hobot_dwmmc_priv *priv,
				   int degrees)
{
	u32 reg_value;

	priv->current_sample_phase = degrees;
	reg_value = readl(HOBOT_MMC_CLK_REG(priv->ctrl_id));
	reg_value &= 0xFFFF0FFF;
	reg_value |= degrees << HOBOT_MMC_SAMPLE_DEGREE_SHIFT;
	writel(reg_value, HOBOT_MMC_CLK_REG(priv->ctrl_id));

	/* We should delay 1us wait for timing setting finished. */
	udelay(1);
	return 0;
}

static int hb_mmc_set_drv_phase(struct hobot_dwmmc_priv *priv,
				   int degrees)
{
	u32 reg_value;

	reg_value = readl(HOBOT_MMC_CLK_REG(priv->ctrl_id));
	reg_value &= 0xFFFFF0FF;
	reg_value |= degrees << HOBOT_MMC_DRV_DEGREE_SHIFT;
	writel(reg_value, HOBOT_MMC_CLK_REG(priv->ctrl_id));

	/* We should delay 1us wait for timing setting finished. */
	udelay(1);
	return 0;
}

static int dw_mci_hb_sample_tuning(struct dwmci_host *host, u32 opcode)
{
	struct udevice *dev = host->priv;
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);
	struct mmc *mmc = host->mmc;
	int ret = 0;
	int i;
	bool v, prev_v = 0, first_v;
	struct range_t {
		int start;
		int end;	/* inclusive */
	};
	struct range_t *ranges;
	unsigned int range_count = 0;
	int longest_range_len = -1;
	int longest_range = -1;
	int middle_phase;

	ranges = calloc(sizeof(*ranges), NUM_PHASES / 2 + 1);
	if (!ranges)
		return -ENOMEM;

	/* Try each phase and extract good ranges */
	for (i = 0; i < NUM_PHASES;) {
		hb_mmc_set_sample_phase(priv, TUNING_ITERATION_TO_PHASE(i));

		v = !mmc_send_tuning(mmc, opcode, NULL);

		if (i == 0)
			first_v = v;

		if ((!prev_v) && v) {
			range_count++;
			ranges[range_count - 1].start = i;
		}

		if (v) {
			ranges[range_count - 1].end = i;
			i++;
		} else if (i == NUM_PHASES - 1) {
			/* No extra skipping rules if we're at the end */
			i++;
		} else {
			/*
			 * No need to check too close to an invalid
			 * one since testing bad phases is slow.  Skip
			 * 20 degrees.
			 */
			i += 1;

			/* Always test the last one */
			if (i >= NUM_PHASES)
				i = NUM_PHASES - 1;
		}

		prev_v = v;
	}

	if (range_count == 0) {
		debug("All sample phases bad!");
		ret = -EIO;
		goto free;
	}

	/* wrap around case, merge the end points */
	if ((range_count > 1) && first_v && v) {
		ranges[0].start = ranges[range_count - 1].start;
		range_count--;
	}

	if (ranges[0].start == 0 && ranges[0].end == NUM_PHASES - 1) {
		hb_mmc_set_sample_phase(priv, priv->default_sample_phase);
		debug("All sample phases work, using default phase %d.",
			priv->default_sample_phase);
		goto free;
	}

	/* Find the longest range */
	for (i = 0; i < range_count; i++) {
		int len = (ranges[i].end - ranges[i].start + 1);

		if (len < 0)
			len += NUM_PHASES;

		if (longest_range_len < len) {
			longest_range_len = len;
			longest_range = i;
		}

		debug(
			"Good sample phase range %d-%d (%d len)\n",
			TUNING_ITERATION_TO_PHASE(ranges[i].start),
			TUNING_ITERATION_TO_PHASE(ranges[i].end), len);
	}

	debug(
		"Best sample phase range %d-%d (%d len)\n",
		TUNING_ITERATION_TO_PHASE(ranges[longest_range].start),
		TUNING_ITERATION_TO_PHASE(ranges[longest_range].end),
		longest_range_len);

	middle_phase = ranges[longest_range].start + longest_range_len / 2;
	middle_phase %= NUM_PHASES;

	hb_mmc_set_sample_phase(priv, TUNING_ITERATION_TO_PHASE(middle_phase));

free:
	free(ranges);
	return ret;
}

static int dw_mci_hb_execute_tuning(struct dwmci_host *host, u32 opcode)
{
	int ret = -EIO;
	struct udevice *dev = host->priv;
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);

	ret = dw_mci_hb_sample_tuning(host, opcode);

	/* uboot default drv_phase is 0 */
	hb_mmc_set_drv_phase(priv, 0);
	return ret;
}
#endif

static int hobot_dwmmc_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct hobot_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;
	int ret, devnum = -1;

	ret = dev_read_alias_seq(dev, &devnum);
	if (ret) {
		/* use non-removeable as sdcard and emmc as judgement */
		if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev), "non-removable"))
			host->dev_index = 0;
		else
			host->dev_index = 1;
	} else {
		host->dev_index = devnum;
	}
	priv->ctrl_id = devnum;
	debug("%s ret=%d devnum=%d host->dev_index=%d\n",
		__func__, ret, devnum, host->dev_index);
	sdio_reset(host->dev_index);
	sdio_pin_mux_config(host->dev_index);
	sdio_power(host->dev_index);

	if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev), "mmc-hs200-1_8v"))
		host->caps = MMC_CAP(MMC_HS_200);

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	host->buswidth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
					"bus-width", 4);
	host->get_mmc_clk = hobot_dwmmc_get_mmc_clk;
	host->priv = dev;

	priv->fifo_depth = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
				    "fifo-depth", 0);
	if (priv->fifo_depth < 0)
		return -EINVAL;
	priv->fifo_mode = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
                                     "fifo-mode", 0);
	host->bus_hz = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
                                     "clock-frequency", 0);
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
	unsigned long clock __attribute__((unused));
	int ret;

#if CONFIG_IS_ENABLED(OF_PLATDATA)

	struct dtd_hobot_hb_dw_mshc *dtplat = &plat->dtplat;

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
#if !defined(CONFIG_TARGET_X2_FPGA) && !defined(CONFIG_TARGET_X3_FPGA)
	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0) {
		debug("failed to get gate clk.\n");
		return ret;
	}
	clock = hobot_dwmmc_get_mmc_clk(host, host->bus_hz);
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
#ifdef MMC_SUPPORTS_TUNING
	host->execute_tuning = dw_mci_hb_execute_tuning;
#endif
	priv->default_sample_phase = 0;
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

	ret = dwmci_probe(dev);
	if (ret)
		debug("dwmci_probe fail\n");

	ret = mmc_init(host->mmc);
	if (ret)
		debug("mmc_init fail\n");

	mmc_set_rst_n_function(host->mmc, 0x01);

    return ret;
}

static int hobot_dwmmc_bind(struct udevice *dev)
{
	struct hobot_mmc_plat *plat = dev_get_platdata(dev);
	return dwmci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id hobot_dwmmc_ids[] = {
	{ .compatible = "hobot,hb-dw-mshc" },
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
