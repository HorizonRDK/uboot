
#include <common.h>
#include <errno.h>
#include <asm/arch/ddr.h>
#include <asm/arch/ddr_phy.h>
#include <asm/arch/clock.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/hb_share.h>
#include <asm/arch/hb_dev.h>

#define IMEM_LEN 32768 /* byte */
#define DMEM_LEN 16384 /* byte */

#define IMEM_OFFSET_ADDR 	(0x00050000 * 4)
#define DMEM_OFFSET_ADDR 	(0x00054000 * 4)

//static unsigned int g_ddr_rate = 3200;
static unsigned int g_ddr_rate = 2666;
struct dram_timing_info dram_timing;

static void lpddr4_ate_load_fw(unsigned long dest,
	unsigned long src, unsigned int fw_size, unsigned int fw_max_size)
{
	unsigned long fw_dest = dest;
	unsigned long fw_src = src;
	unsigned int tmp32;
	unsigned int i;

	for (i = 0; i < fw_size; ) {
		tmp32 = readl(fw_src);
		writel(tmp32 & 0x0000ffff, fw_dest);
		fw_dest += 4;
		writel((tmp32 >> 16) & 0x0000ffff, fw_dest);
		fw_dest += 4;
		fw_src += 4;
		i += 4;
	}

	for (i = fw_size; i < fw_max_size; i += 2 ) {
		writel(0x0, fw_dest);
		fw_dest += 4;
	}

	return;
}

static void lpddr4_ate_exec_fw(void)
{
	unsigned int value;

	reg32_write(DDRP_DRTUB0_UCTWRITEPROT, 0x1);
    reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x1);

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);
    reg32_write(DDRP_APBONLY0_MICRORESET, 0x9);
    reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);
    reg32_write(DDRP_APBONLY0_MICRORESET, 0x0);

	while (reg32_read(DDRP_APBONLY0_UCTSHADOWREGS) & 0x1);

	reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x0);
	reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x1);

	reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);
    reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);

    value = reg32_read(DDRP_BASE_ADDR + (0x54001<<2));
}

void ddr_init(struct dram_timing_info *dram_timing)
{
	int i;
	volatile unsigned int temp = 0;
	unsigned int fw_src_laddr = 0;
	unsigned int fw_src_len;

	dram_pll_init(MHZ(g_ddr_rate));

	reg32_write(X2_PMU_DDRSYS_CTRL, 0x1);

	for (i = 0; i < 250; i++) {
		temp = i;
    }

	reg32_write(X2_SYSC_DDRSYS_SW_RSTEN, 0x0);

	reg32_write(DDRP_MASTER0_PLLCTRL1_P0, 0x80);
	reg32_write(DDRP_MASTER0_PLLCTRL4_P0, 0x17f);
	reg32_write(DDRP_MASTER0_PLLTESTMODE_P0, 0xb);

	printf("\nATE load fw imem ...\n");
	/* Load 32KB firmware */
	fw_src_len = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, IMEM_LEN);

	lpddr4_ate_load_fw(DDRP_BASE_ADDR + IMEM_OFFSET_ADDR,
		X2_SRAM_LOAD_ADDR, fw_src_len, IMEM_LEN);

	printf("\nATE load fw dmem ...\n");
	/* Load 16KB firmware */
	fw_src_len = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, DMEM_LEN);

	lpddr4_ate_load_fw(DDRP_BASE_ADDR + DMEM_OFFSET_ADDR,
		X2_SRAM_LOAD_ADDR, fw_src_len, DMEM_LEN);

	lpddr4_ate_exec_fw();

	printf("\n=== Complete ddr ate ===\n");
	while (1);
}
