
#include <linux/kernel.h>
#include <common.h>
#include <asm/arch/ddr.h>
#include <asm/arch/ddr_phy.h>

#ifdef CONFIG_X2_LPDDR4_3200
#include "lpddr4_3200.h"

unsigned int g_ddr_rate = 3200;
#endif /* CONFIG_X2_LPDDR4_3200 */

#ifdef CONFIG_X2_LPDDR4_2133
#include "lpddr4_2133.h"

unsigned int g_ddr_rate = 2133;
#endif /* CONFIG_X2_LPDDR4_2133 */

#ifdef CONFIG_X2_LPDDR4_2666
#include "lpddr4_2666.h"

unsigned int g_ddr_rate = 2666;
#endif /* CONFIG_X2_LPDDR4_2666 */

#ifdef CONFIG_X2_LPDDR4_2400
#include "lpddr4_2400.h"

unsigned int g_ddr_rate = 2400;
#endif /* CONFIG_X2_LPDDR4_2400 */

/* lpddr4 timing config params on EVK board */
struct dram_timing_info dram_timing = {
	.ddrc_cfg = lpddr4_ddrc_cfg,
	.ddrc_cfg_num = ARRAY_SIZE(lpddr4_ddrc_cfg),
	.ddrphy_cfg = lpddr4_ddrphy_cfg,
	.ddrphy_cfg_num = ARRAY_SIZE(lpddr4_ddrphy_cfg),
	.fsp_msg = NULL,
	.fsp_msg_num = 0,
	.ddrphy_pie = lpddr4_phy_pie,
	.ddrphy_pie_num = ARRAY_SIZE(lpddr4_phy_pie),
};

