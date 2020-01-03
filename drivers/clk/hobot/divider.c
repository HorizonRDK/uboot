#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

struct div_platdata {
	phys_addr_t div_reg;
	uint bits;
	uint field;
};

static ulong div_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	uint div;
	int ret, val;

	struct div_platdata *plat = dev_get_platdata(clk->dev);
	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret){
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	val = readl(plat->div_reg);
	div = (val & (plat->field << plat->bits)) >> plat->bits;

	clk_rate = clk_rate / (div + 1);
	/*CLK_DEBUG("div clk rate:%ld, div:0x%x.\n", clk_rate, div);*/

	return clk_rate;
}

/*Notice: do not gurantee the requested rate, pls check after set rate.*/
ulong div_clk_set_rate(struct clk *clk, ulong rate)
{
	struct clk source;
	ulong clk_rate;
	uint div, val, ret;
	struct div_platdata *plat = dev_get_platdata(clk->dev);

	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret){
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	if(rate > clk_rate){
		CLK_DEBUG("Invalid clock rate request.\n");
		return -1;
	}else{
		div = clk_rate / rate - 1;
	}

	if(div > (0x1 << plat->field)){
		div = plat->field;
	}

	val = readl(plat->div_reg);
	val &= ~(plat->field << plat->bits);
	val |= ((div & plat->field) << plat->bits);
	writel(val, plat->div_reg);
	/*CLK_DEBUG("div set clk rate:%ld, val:0x%x, addr:0x%llx.\n",
			rate, val, plat->div_reg);*/

	return 0;
}

static struct clk_ops div_clk_ops = {
	.get_rate = div_clk_get_rate,
	.set_rate = div_clk_set_rate,
};

static int div_clk_probe(struct udevice *dev)
{
	uint val;
	phys_addr_t reg_base;
	ofnode node;
	struct div_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32(dev->node, "offset", &val)) {
		CLK_DEBUG("Node '%s' has missing 'offset' property!\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->div_reg = reg_base + val;

	if(ofnode_read_u32(dev->node, "bits", &val)) {
		CLK_DEBUG("Node '%s' has missing 'bits' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->bits = val;

	if(ofnode_read_u32(dev->node, "field", &val)) {
		CLK_DEBUG("Node '%s' has missing 'field' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->field = (1 << val) - 1;

	/*CLK_DEBUG("div %s probe done, div:0x%x, bits:0x%x, field:0x%x\n",
			dev->name, plat->div_reg, plat->bits, plat->field);*/
	return 0;
}

static const struct udevice_id div_clk_match[] = {
	{ .compatible = "hobot,div-clk"},
	{}
};

U_BOOT_DRIVER(div_clk) = {
	.name = "div_clk",
	.id = UCLASS_CLK,
	.of_match  = div_clk_match,
	.probe = div_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct div_platdata),
	.ops = &div_clk_ops,
};
