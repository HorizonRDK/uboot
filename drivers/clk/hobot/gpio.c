/*
 *    COPYRIGHT NOTICE
 *   Copyright 2020 Horizon Robotics, Inc.
 *    All rights reserved.
*/

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

struct gpio_platdata {
	phys_addr_t reg;
	uint bit;
};

int gpio_clk_enable(struct clk *clk)
{
	unsigned int val = 0, status = 0, reg_val = 0;
	struct gpio_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->reg);
	status = (val & (1 << plat->bit)) >> plat->bit;

	if (status) {
		reg_val &= ~(1 << plat->bit);
		writel(reg_val, plat->reg);
		/*CLK_DEBUG("gpio-clk enable, val:0x%x, reg:0x%x", plat->bit, plat->reg);*/
	}
	return 0;
}

int gpio_clk_disable(struct clk *clk)
{
	unsigned int val = 0, status = 0, reg_val = 0;
	struct gpio_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->reg);
	status = (val & (1 << plat->bit)) >> plat->bit;

	if (!status) {
		reg_val |= 1 << plat->bit;
		writel(reg_val, plat->reg);
	/*CLK_DEBUG("gate disable, val:0x%x, reg:0x%x", plat->bit, plat->reg);*/
	}
	return 0;
}

static ulong gpio_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	int ret;

	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret) {
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	/*CLK_DEBUG("gpio clock rate:%ld.\n", clk_rate);*/
	return clk_rate;
}

static struct clk_ops gpio_clk_ops = {
	.get_rate = gpio_clk_get_rate,
	.disable = gpio_clk_disable,
	.enable = gpio_clk_enable,
};

static int gpio_clk_probe(struct udevice *dev)
{
	uint val;
	phys_addr_t reg_base;
	ofnode node;
	struct gpio_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32(dev->node, "offset", &val)) {
		CLK_DEBUG("Node '%s' has missing 'offset' property!\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->reg = reg_base + val;

	if(ofnode_read_u32(dev->node, "bits", &val)) {
		CLK_DEBUG("Node '%s' has missing 'bits' property\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->bit = val;

	CLK_DEBUG("gpio-clk %s probe done, reg:0x%llx, bit:0x%llx\n",
			dev->name, plat->reg, plat->bit);
	return 0;
}

static const struct udevice_id gpio_clk_match[] = {
	{ .compatible = "hobot,gpio-clk"},
	{}
};

U_BOOT_DRIVER(gpio_clk) = {
	.name = "gpio_clk",
	.id = UCLASS_CLK,
	.of_match  = gpio_clk_match,
	.probe = gpio_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct gpio_platdata),
	.ops = &gpio_clk_ops,
};
