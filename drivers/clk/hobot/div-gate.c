#include "div-comm.h"

struct div_gate_platdata        {
	struct div_platdata div_reg;
	struct gate_platdata gate_reg;
};

int div_gate_clk_enable(struct clk *clk)
{
	struct div_gate_platdata *plat = dev_get_platdata(clk->dev);
	unsigned int val, status, reg_val;

	val = readl(plat->gate_reg.clken_sta_reg);
	status = (val & (1 << plat->gate_reg.clken_sta_bit))
	>> plat->gate_reg.clken_sta_bit;

	if(plat->gate_reg.disable_bit == 32) {
		if (!status) {
		val |= 1 << plat->gate_reg.enable_bit;
		writel(val, plat->gate_reg.enable_reg);
		}
	} else {
		if (!status) {
		reg_val = 1 << plat->gate_reg.enable_bit;
		writel(reg_val, plat->gate_reg.enable_reg);
		}
	}

	return 0;
}

int div_gate_clk_disable(struct clk *clk)
{
	struct div_gate_platdata *plat = dev_get_platdata(clk->dev);
	unsigned int val, status, reg_val;

	val = readl(plat->gate_reg.clken_sta_reg);
	status = (val & (1 << plat->gate_reg.clken_sta_bit))
	>> plat->gate_reg.clken_sta_bit;
	/* determining is ips reg? */
	if(plat->gate_reg.disable_bit == 32) {
		if (status) {
			val |= 0 << plat->gate_reg.enable_bit;
			writel(val, plat->gate_reg.enable_reg);
		}
	} else {
		if (status) {
			reg_val = 1 << plat->gate_reg.disable_bit;
			writel(reg_val, plat->gate_reg.disable_reg);
		}
	}

	return 0;
}

static ulong div_gate_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	uint div;
	int ret, val;

	struct div_gate_platdata *plat = dev_get_platdata(clk->dev);
	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret) {
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	val = readl(plat->div_reg.divider_reg);
	div = (val & (div_mask(plat->div_reg.div_field) << plat->div_reg.div_bits))
		>> plat->div_reg.div_bits;

	clk_rate = clk_rate / (div + 1);
	/*CLK_DEBUG("div clk rate:%ld, div:0x%x.\n", clk_rate, div);*/

	return clk_rate;
}

/*Notice: do not gurantee the requested rate, pls check after set rate.*/
ulong div_gate_clk_set_rate(struct clk *clk, ulong rate)
{
	struct div_gate_platdata *plat = dev_get_platdata(clk->dev);
	struct clk source;
	ulong clk_rate;
	uint div, val, ret;
	int reg_val, reg0_val, reg1_val;

	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret) {
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	if(rate > clk_rate) {
		CLK_DEBUG("Invalid clock rate request.\n");
		return -1;
	} else {
		div = clk_rate / rate - 1;
	}

	if(div > div_mask(plat->div_reg.div_field)) {
		div = div_mask(plat->div_reg.div_field);
	}

	/* disable clk */
	if(plat->gate_reg.disable_bit == 32) {
		reg_val = readl(plat->gate_reg.clken_sta_reg);
		reg_val |= 0 << plat->gate_reg.enable_bit;
		writel(reg_val, plat->gate_reg.enable_reg);
	} else {
		reg0_val = 1 << plat->gate_reg.disable_bit;
		writel(reg0_val, plat->gate_reg.disable_reg);
	}

	val = readl(plat->div_reg.divider_reg);
	val &= ~(div_mask(plat->div_reg.div_field) << plat->div_reg.div_bits);
	val |= div << plat->div_reg.div_bits;
	writel(val, plat->div_reg.divider_reg);

	/* enable clk */
	if(plat->gate_reg.disable_bit == 32) {
		reg_val = readl(plat->gate_reg.clken_sta_reg);
		reg_val |= 1 << plat->gate_reg.enable_bit;
		writel(reg_val, plat->gate_reg.enable_reg);
	} else {
		reg1_val = 1 << plat->gate_reg.enable_bit;
		writel(reg1_val, plat->gate_reg.enable_reg);
	}

	/*CLK_DEBUG("div set clk rate:%ld, val:0x%x, addr:0x%llx.\n",
			rate, val, plat->div_reg);*/

	return 0;
}

const struct clk_ops div_gate_clk_ops = {
	.enable = &div_gate_clk_enable,
	.disable = &div_gate_clk_disable,

	.get_rate = &div_gate_clk_get_rate,
	.set_rate = &div_gate_clk_set_rate,
};

static int div_gate_clk_probe(struct udevice *dev)
{
	uint reg[4];
	phys_addr_t reg_base, vioreg_base;
	ofnode node;
	struct div_gate_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);
	if (!reg_base) {
		CLK_DEBUG("Node '%s' failed to get the 'reg base'!\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}

	vioreg_base = ofnode_get_addr_index(node, 1);
	if (!vioreg_base) {
		CLK_DEBUG("Node '%s' failed to get the 'vio_reg base'!\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}

	if(ofnode_read_u32_array(dev->node, "bits", reg, 4)) {
		CLK_DEBUG("Node '%s' has missing 'bits' property\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->div_reg.div_bits = reg[0];
	plat->gate_reg.clken_sta_bit = reg[1];
	plat->gate_reg.enable_bit = reg[2];
	plat->gate_reg.disable_bit = reg[3];

	if (plat->gate_reg.disable_bit != 32) {
		if(ofnode_read_u32_array(dev->node, "offset", reg, 4)) {
			CLK_DEBUG("Node '%s' has missing 'offset' property\n",
			ofnode_get_name(dev->node));
			return -ENOENT;
		}
		plat->div_reg.divider_reg = reg_base + reg[0];
		plat->gate_reg.clken_sta_reg = reg_base + reg[1];
		plat->gate_reg.enable_reg = reg_base + reg[2];
		plat->gate_reg.disable_reg = reg_base + reg[3];
	} else {
		if(ofnode_read_u32_array(dev->node, "offset", reg, 4)) {
			CLK_DEBUG("Node '%s' has missing 'offset' property\n",
				ofnode_get_name(dev->node));
			return -ENOENT;
		}
		plat->div_reg.divider_reg = reg_base + reg[0];
		plat->gate_reg.clken_sta_reg = vioreg_base + reg[1];
		plat->gate_reg.enable_reg = vioreg_base + reg[2];
		plat->gate_reg.disable_reg = vioreg_base + reg[3];
	}

	if(ofnode_read_u32_array(dev->node, "field", reg, 4)) {
		CLK_DEBUG("Node '%s' has missing 'field' property\n",
			ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->div_reg.div_field = reg[0];
	plat->gate_reg.clken_sta_field = reg[1];
	plat->gate_reg.enable_field = reg[2];
	plat->gate_reg.disable_field = reg[3];

	/*CLK_DEBUG("div %s probe done, div:0x%x, ebreg:0x%x\n", dev->name,
			plat->div_reg.divider_reg, plat->gate_reg.clken_sta_reg);*/
	return 0;
}

static const struct udevice_id div_gate_clk_match[] = {
	{ .compatible = "hobot,div-gate-clk"},
	{}
	};

U_BOOT_DRIVER(div_gate_clk) = {
	.name = "div_gate_clk",
	.id = UCLASS_CLK,
	.of_match  = div_gate_clk_match,
	.probe = div_gate_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct div_gate_platdata),
	.ops = &div_gate_clk_ops,
	};

