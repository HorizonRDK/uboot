#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

struct fixed_div_platdata {
	uint div;
	uint mult;
};

static ulong fixed_div_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	uint ret;
	struct fixed_div_platdata *plat = dev_get_platdata(clk->dev);

	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret){
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	clk_rate = clk_rate * plat->mult / plat->div;

	/*CLK_DEBUG("fixed div clk rate:%ld, div:%d, mult:%d.\n",
			clk_rate, plat->div, plat->mult);*/

	return clk_rate;
}


static struct clk_ops fixed_div_clk_ops = {
	.get_rate = fixed_div_clk_get_rate,
};

static int fixed_div_clk_probe(struct udevice *dev)
{
	u32 val;
	struct fixed_div_platdata *plat = dev_get_platdata(dev);

	if(ofnode_read_u32(dev->node, "clock-div", &val)) {
		CLK_DEBUG("Node '%s' has missing 'clock-div' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->div = val;

	if(ofnode_read_u32(dev->node, "clock-mult", &val)) {
		CLK_DEBUG("Node '%s' has missing 'clock-mult' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}
	plat->mult = val;

	/*CLK_DEBUG("fixed factor divider %s probe done, div:%d, mult:%d.\n",
			ofnode_get_name(dev->node), plat->div, plat->mult);*/
	return 0;
}

static const struct udevice_id fixed_div_clk_match[] = {
	{ .compatible = "hobot,fixed-factor-clk"},
	{}
};

U_BOOT_DRIVER(fixed_div_clk) = {
	.name = "fixed_div_clk",
	.id = UCLASS_CLK,
	.of_match  = fixed_div_clk_match,
	.probe = fixed_div_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct fixed_div_platdata),
	.ops = &fixed_div_clk_ops,
};
