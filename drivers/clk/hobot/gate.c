#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

struct gate_platdata {
	uint state_reg;
	uint state_bits;
	uint enable_reg;
	uint enable_bits;
	uint disable_reg;
	uint disable_bits;
};

int gate_clk_enable(struct clk *clk)
{
	unsigned int val, status;
	struct gate_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->state_reg);
	status = (val & (0x1 << plat->state_bits)) >> plat->state_bits;

	if(!status){
		writel((0x1 << plat->enable_bits), plat->enable_reg);
		/*CLK_DEBUG("gate enable, val:0x%x, reg:0x%x", plat->enable_bits, plat->enable_reg);*/
	}

	return 0;
}

int gate_clk_disable(struct clk *clk)
{
	unsigned int val, status;
	struct gate_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->state_reg);
	status = (val & (0x1 << plat->state_bits)) >> plat->state_bits;

	if(status){
		writel((0x1 << plat->disable_bits), plat->disable_reg);
		/*CLK_DEBUG("gate disable, val:0x%x, reg:0x%x", plat->disable_bits, plat->disable_reg);*/
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
	uint reg[3];
	uint reg_base;
	ofnode node;
	struct gate_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32_array(dev->node, "offset", reg, 3)) {
		CLK_DEBUG("Node '%s' has missing 'offset' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->state_reg = reg_base + reg[0];
	plat->enable_reg = reg_base + reg[1];
	plat->disable_reg = reg_base + reg[2];

	if(ofnode_read_u32_array(dev->node, "bits", reg, 3)) {
		CLK_DEBUG("Node '%s' has missing 'bits' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->state_bits = reg[0];
	plat->enable_bits = reg[1];
	plat->disable_bits = reg[2];

	/*CLK_DEBUG("divider %s probe done, state:0x%x, enable:0x%x, disable:0x%x\n",
			dev->name, plat->state_reg, plat->enable_reg, plat->disable_reg);*/
	return 0;
}

static const struct udevice_id gate_clk_match[] = {
	{ .compatible = "x2,gate-clk"},
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
