
#include <asm/arch/hb_pmu.h>
#include <asm/io.h>
#include <common.h>

void reset_cpu(ulong addr)
{
	/*SPL will jump to warmboot when this value is not 0*/
	writel(0x0, HB_PMU_SW_REG_19);
	writel(0x00001001, HB_PMU_POWER_CTRL);
	while (1);
}
