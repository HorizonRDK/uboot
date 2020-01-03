#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"
struct mux_platdata {
	phys_addr_t mux_reg;
	uint bits;
	uint field;
};

static ulong mux_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	uint ret, val, mux_val;
	struct mux_platdata *plat = dev_get_platdata(clk->dev);

	val = readl(plat->mux_reg);
	mux_val = (val & (plat->field << plat->bits)) >> plat->bits;

	ret = clk_get_by_index(clk->dev, mux_val, &source);
	if(ret){
		CLK_DEBUG("Failed to get %d source clock.\n", mux_val);
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);
	/*CLK_DEBUG("mux clk rate:%ld, mux:%d.\n", clk_rate, mux_val);*/
	return clk_rate;
}

static int mux_clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct clk source;
	uint mux_sel, mux_num, ret, val;
	struct mux_platdata *plat = dev_get_platdata(clk->dev);

	mux_num = 0x1 << plat->field;
	for(mux_sel=0; mux_sel<=mux_num; mux_num++){
		ret = clk_get_by_index(clk->dev, mux_sel, &source);
		if(ret){
			CLK_DEBUG("Failed to get %d source clock.\n", mux_sel);
			return -EINVAL;
		}
		if(!strcmp(parent->dev->name, source.dev->name))
			break;
	}

	if(mux_sel <= mux_num){
		val = readl(plat->mux_reg);
		val &= ~(plat->field << plat->bits);
		val |= ((mux_sel & plat->field) << plat->bits);
		writel(val, plat->mux_reg);
		/*CLK_DEBUG("set parent index:%d, val:0x%x, reg:0x%x.\n",
				mux_sel, val, plat->mux_reg);*/
		return 0;
	}else{
		return -1;
	}
}

static struct clk_ops mux_clk_ops = {
	.get_rate = mux_clk_get_rate,
	.set_parent = mux_clk_set_parent,
};

static int mux_clk_probe(struct udevice *dev)
{
	uint tmp;
	phys_addr_t reg_base;
	ofnode node;
	struct mux_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32(dev->node, "offset", &tmp)){
		CLK_DEBUG("Node %s has missing 'offset' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->mux_reg = reg_base + tmp;

	if(ofnode_read_u32(dev->node, "bits", &tmp)){
		CLK_DEBUG("Node %s has missing 'bits' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->bits = tmp;

	if(ofnode_read_u32(dev->node, "field", &tmp)){
		CLK_DEBUG("Node %s has missing 'field' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->field = tmp;

	/*CLK_DEBUG("mux %s probe done, mux:0x%x, bits:0x%x, field:0x%x.\n",
			dev->name, plat->mux_reg, plat->bits, plat->field);*/
	return 0;
}

static const struct udevice_id mux_clk_match[] = {
	{ .compatible = "hobot,mux-clk"},
	{}
};

U_BOOT_DRIVER(mux_clk) = {
	.name = "mux_clk",
	.id = UCLASS_CLK,
	.of_match = mux_clk_match,
	.probe = mux_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct mux_platdata),
	.ops = &mux_clk_ops,
};
