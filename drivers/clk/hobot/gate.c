#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

struct gate_platdata {
	phys_addr_t clken_sta_reg;
	uint clken_sta_bit;
	uint clken_sta_field;
	phys_addr_t clkoff_sta_reg;
	uint clkoff_sta_bit;
	uint clkoff_sta_field;
	phys_addr_t enable_reg;
	uint enable_bit;
	uint enable_field;
	phys_addr_t disable_reg;
	uint disable_bit;
	uint disable_field;
};

int gate_clk_enable(struct clk *clk)
{
	unsigned int val, val1, status0, status1, reg_val;
	struct gate_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->clken_sta_reg);
	status0 = (val & (1 << plat->clken_sta_bit)) >> plat->clken_sta_bit;

	/* determining if clkoff_sta_reg exists */
	if (plat->clkoff_sta_bit != 32) {
		val1 = readl(plat->clkoff_sta_reg);
		status1 = (val1 & (1 << plat->clkoff_sta_bit))
		>> plat->clkoff_sta_bit;

		if (status0 == 0 || status1 == 1) {
			reg_val = 1 << plat->enable_bit;
			writel(reg_val, plat->enable_reg);
		/*CLK_DEBUG("gate enable, val:0x%x, reg:0x%x",
			plat->enable_bit, plat->enable_reg);*/
		}
	} else {
		if (!status0) {
			reg_val = 1 << plat->enable_bit;
			writel(reg_val, plat->enable_reg);
		/*CLK_DEBUG("gate enable, val:0x%x, reg:0x%x",
			plat->enable_bit, plat->enable_reg);*/
		}
	}
	return 0;
}

int gate_clk_disable(struct clk *clk)
{
	unsigned int val, val1, status0, status1, reg_val;
	struct gate_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->clken_sta_reg);
	status0 = (val & (1 << plat->clken_sta_bit)) >> plat->clken_sta_bit;
	/* determining if clkoff_sta_reg exists */
	if (plat->clkoff_sta_bit != 32) {
		val1 = readl(plat->clkoff_sta_reg);
		status1 = (val1 & (1 << plat->clkoff_sta_bit))
		>> plat->clkoff_sta_bit;

		if (status0 == 1 && status1 == 0) {
			reg_val = 1 << plat->disable_bit;
			writel(reg_val, plat->disable_reg);
		/*CLK_DEBUG("gate disable, val:0x%x, reg:0x%x",
			plat->disable_bits, plat->disable_reg);*/
		}
	} else {
		if (status0) {
			reg_val = 1 << plat->disable_bit;
			writel(reg_val, plat->disable_reg);
		/*CLK_DEBUG("gate disable, val:0x%x, reg:0x%x",
			plat->disable_bit, plat->disable_reg);*/
		}
	}
	return 0;
}

static ulong gate_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	int ret;

	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret){
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	/*CLK_DEBUG("gate clock rate:%ld.\n", clk_rate);*/
	return clk_rate;
}

static struct clk_ops gate_clk_ops = {
	.get_rate = gate_clk_get_rate,
	.enable = gate_clk_enable,
	.disable = gate_clk_disable,
};

static int gate_clk_probe(struct udevice *dev)
{
	uint reg[4];
	phys_addr_t reg_base;
	ofnode node;
	struct gate_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32_array(dev->node, "offset", reg, 4)) {
		CLK_DEBUG("Node '%s' has missing 'offset' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->clken_sta_reg = reg_base + reg[0];
	plat->enable_reg = reg_base + reg[1];
	plat->disable_reg = reg_base + reg[2];
	plat->clkoff_sta_reg = reg_base + reg[3];

	if(ofnode_read_u32_array(dev->node, "bits", reg, 4)) {
		CLK_DEBUG("Node '%s' has missing 'bits' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->clken_sta_bit = reg[0];
	plat->enable_bit = reg[1];
	plat->disable_bit = reg[2];
	plat->clkoff_sta_bit = reg[3];

	if (plat->clkoff_sta_reg != reg_base) {
		if(ofnode_read_u32_array(dev->node, "field", reg, 4)) {
			CLK_DEBUG("Node '%s' has missing 'field' property\n",
				ofnode_get_name(dev->node));
			return -ENOENT;
		}
		plat->clken_sta_field = reg[0];
		plat->enable_field = reg[1];
		plat->disable_field = reg[2];
		plat->clkoff_sta_field = reg[3];
	} else {
		if(ofnode_read_u32_array(dev->node, "field", reg, 3)) {
			CLK_DEBUG("Node '%s' has missing 'field' property\n",
				ofnode_get_name(dev->node));
			return -ENOENT;
		}
		plat->clken_sta_field = reg[0];
		plat->enable_field = reg[1];
		plat->disable_field = reg[2];
	}

	CLK_DEBUG("gate %s probe done, state:0x%llx, enable:0x%llx, disable:0x%llx\n",
			dev->name, plat->clken_sta_reg, plat->enable_reg, plat->disable_reg);
	return 0;
}

static const struct udevice_id gate_clk_match[] = {
	{ .compatible = "hobot,gate-clk"},
	{}
};

U_BOOT_DRIVER(gate_clk) = {
	.name = "gate_clk",
	.id = UCLASS_CLK,
	.of_match  = gate_clk_match,
	.probe = gate_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct gate_platdata),
	.ops = &gate_clk_ops,
};
