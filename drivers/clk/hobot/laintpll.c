#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"
#define PLL_FREQ_CTRL_FBDIV_BIT 0
#define PLL_FREQ_CTRL_REFDIV_BIT 12
#define PLL_FREQ_CTRL_POSTDIV1_BIT 20
#define PLL_FREQ_CTRL_POSTDIV2_BIT 24

#define PLL_FREQ_CTRL_FBDIV_FIELD 0xFFF
#define PLL_FREQ_CTRL_REFDIV_FIELD 0x3F
#define PLL_FREQ_CTRL_POSTDIV1_FIELD 0x7
#define PLL_FREQ_CTRL_POSTDIV2_FIELD 0x7

#define PLL_PD_CTRL_PD_BIT 0
#define PLL_PD_CTRL_DSMPD_BIT 4
#define PLL_PD_CTRL_FOUTPOSTDIVPD_BIT 8
#define PLL_PD_CTRL_FOUTVCOPD_BIT 12
#define PLL_PD_CTRL_BYPASS_BIT 16

#define PLL_BYPASS_MODE 0x1
#define PLL_LAINT_MODE 0x0

#define PLL_LAINT_REFDIV_MIN 1
#define PLL_LAINT_REFDIV_MAX 63
#define PLL_LAINT_FBDIV_MIN 16
#define PLL_LAINT_FBDIV_MAX 320
#define PLL_LAINT_POSTDIV1_MIN 1
#define PLL_LAINT_POSTDIV1_MAX 7
#define PLL_LAINT_POSTDIV2_MIN 1
#define PLL_LAINT_POSTDIV2_MAX 7

struct laintpll_platdata {
	phys_addr_t freq_reg;
	phys_addr_t pd_reg;
};

static ulong laintpll_clk_get_rate(struct clk *clk)
{
	struct clk source;
	ulong clk_rate;
	uint fbdiv, refdiv, postdiv1, postdiv2;
	uint ret, val;

	struct laintpll_platdata *plat = dev_get_platdata(clk->dev);
	ret = clk_get_by_index(clk->dev, 0, &source);
	if(ret){
		CLK_DEBUG("Failed to get source clk.\n");
		return -EINVAL;
	}

	clk_rate = clk_get_rate(&source);

	val = readl(plat->freq_reg);
	fbdiv = (val & (PLL_FREQ_CTRL_FBDIV_FIELD << PLL_FREQ_CTRL_FBDIV_BIT)) >> PLL_FREQ_CTRL_FBDIV_BIT;
	refdiv = (val & (PLL_FREQ_CTRL_REFDIV_FIELD << PLL_FREQ_CTRL_REFDIV_BIT)) >> PLL_FREQ_CTRL_REFDIV_BIT;
	postdiv1 = (val & (PLL_FREQ_CTRL_POSTDIV1_FIELD << PLL_FREQ_CTRL_POSTDIV1_BIT)) >> PLL_FREQ_CTRL_POSTDIV1_BIT;
	postdiv2 = (val & (PLL_FREQ_CTRL_POSTDIV2_FIELD << PLL_FREQ_CTRL_POSTDIV2_BIT)) >> PLL_FREQ_CTRL_POSTDIV2_BIT;

	clk_rate = (clk_rate / refdiv) * fbdiv / postdiv1 /postdiv2;
	/*CLK_DEBUG("laintpll clk rate:%ld, refdiv:%d, fbdiv:%d, postdiv1:%d, postdiv2:%d.\n",
			clk_rate, refdiv, fbdiv, postdiv1, postdiv2);*/
	return clk_rate;
}

const struct clk_ops laintpll_clk_ops = {
	.get_rate = laintpll_clk_get_rate,
};

static int laintpll_clk_probe(struct udevice *dev)
{
	uint reg[2];
	phys_addr_t reg_base;
	ofnode node;
	struct laintpll_platdata *plat = dev_get_platdata(dev);

	node = ofnode_get_parent(dev->node);
	if (!ofnode_valid(node)) {
		CLK_DEBUG("Failed to get parent node!\n");
		return -EINVAL;
	}

	reg_base = ofnode_get_addr(node);

	if(ofnode_read_u32_array(dev->node, "offset", reg, 2)) {
		CLK_DEBUG("Node '%s' has missing 'offset' property\n", ofnode_get_name(dev->node));
		return -ENOENT;
	}

	plat->freq_reg = reg_base + reg[0];
	plat->pd_reg = reg_base + reg[1];

	/*CLK_DEBUG("laintpll:%s probe done, freq:0x%x, pd:0x%x.\n",
			dev->name, plat->freq_reg, plat->pd_reg);*/
	return 0;
}

static const struct udevice_id laintpll_clk_match[] = {
	{ .compatible = "hobot,laintpll-clk"},
	{}
};

U_BOOT_DRIVER(laintpll_clk) = {
	.name = "laintpll_clk",
	.id = UCLASS_CLK,
	.of_match  = laintpll_clk_match,
	.probe = laintpll_clk_probe,
	.platdata_auto_alloc_size = sizeof(struct laintpll_platdata),
	.ops = &laintpll_clk_ops,
};
