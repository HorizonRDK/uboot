
#include <asm/arch/x2_pmu.h>
#include <asm/io.h>
#include <common.h>

void reset_cpu(ulong addr)
{
	writel(0x00001001, X2_PMU_POWER_CTRL);
	while (1);
}
