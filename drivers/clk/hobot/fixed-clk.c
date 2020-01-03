#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"
struct fixed_platdata {
	ulong freq;
};

static ulong fixed_clk_get_rate(struct clk *clk)
{
	struct fixed_platdata *plat = dev_get_platdata(clk->dev);
	/*CLK_DEBUG("fixed clk rate:%ld.\n", plat->freq);*/
	return plat->freq;
}


static struct clk_ops fixed_clk_ops = {
	.get_rate = fixed_clk_get_rate,
};

static int fixed_clk_probe(struct udevice *dev)
{
	u32 val;
	struct fixed_platdata *plat = dev_get_platdata(dev);

	if(ofnode_read_u32(dev->node, "clock-freq", &val)) {
		CLK_DEBUG("Node '%s' has missing 'clock-freq' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}

	plat->freq = val;
	/*CLK_DEBUG("fixed clk %s probe done, freq:%ld.\n", dev->name, plat->freq);*/
	return 0;
}

static const struct udevice_id fixed_clk_match[] = {
	{ .compatible = "hobot,fixed-clk"},
	{}
};

U_BOOT_DRIVER(fixed_clk) = {
	.name = "fixed_clk",
	.id = UCLASS_CLK,
	.of_match  = fixed_clk_match,
	.probe = fixed_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct fixed_platdata),
	.ops = &fixed_clk_ops,
};
