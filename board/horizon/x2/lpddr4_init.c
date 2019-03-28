
#include <common.h>
#include <errno.h>
#include <asm/arch/ddr.h>
#include <asm/arch/ddr_phy.h>
#include <asm/arch/clock.h>
#include <asm/arch/x2_pmu.h>
#include <asm/arch/x2_sysctrl.h>
#include <asm/arch/x2_share.h>
#include <asm/arch/x2_dev.h>

#if 0
#define LPDDR4_DEBUG	1
#endif /* #if 0 */

#define IMEM_LEN 32768 /* byte */
#define DMEM_LEN 16384 /* byte */

#define IMEM_OFFSET_ADDR 	(0x00050000 * 4)
#define DMEM_OFFSET_ADDR 	(0x00054000 * 4)

extern struct x2_info_hdr g_binfo;

static void lpddr4_cfg_umctl2(struct dram_cfg_param *ddrc_cfg, int num)
{
	int i = 0;

	for (i = 0; i < num; i++) {
		reg32_write(ddrc_cfg->reg, ddrc_cfg->val);
		ddrc_cfg++;
	}
}

static void lpddr4_cfg_phy(struct dram_timing_info *dram_timing)
{
	int i = 0;
	struct dram_cfg_param *ddrp_cfg = dram_timing->ddrphy_cfg;

	for (i = 0; i < dram_timing->ddrphy_cfg_num; i++) {
		reg32_write(ddrp_cfg->reg, ddrp_cfg->val);
		ddrp_cfg++;
	}
}

static void lpddr4_load_fw(unsigned long dest,
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

#ifdef LPDDR4_DEBUG
static inline unsigned int lpddr4_get_mail(int dbg_en)
{
	unsigned int value;
	unsigned int temp;

	while ((reg32_read(DDRP_APBONLY0_UCTSHADOWREGS) & 0x1));

	value = reg32_read(DDRP_APBONLY0_UCTWRITEONLYSHADOW);
	printf("wonlysha = 0x%x\n", value);

	if (dbg_en > 0) {
		temp = reg32_read(DDRP_APBONLY0_UCTDATWRITEONLYSHADOW);
		value |= temp << 16;
		printf("dwonlysha = 0x%x, ret = 0x%x\n", temp, value);
	}

	reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x0);

	while (!(reg32_read(DDRP_APBONLY0_UCTSHADOWREGS) & 0x1));

	reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x1);

	return value;
}

static void lpddr4_exec_fw(void)
{
	unsigned int major_msg;
	unsigned int str_index;
	unsigned int loop_max;

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x9);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x0);

	while ((major_msg = lpddr4_get_mail(0)) != 0x7) {
		if (major_msg == 0x8) {
			str_index = lpddr4_get_mail(1);
			loop_max = str_index & 0xffff;

			for (int i = 0; i < loop_max; i++) {
				printf("arg: %d\n", i + 1);
				lpddr4_get_mail(1);
			}
		}
	}

	reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);

	return;
}
#else
static void lpddr4_exec_fw(void)
{
	unsigned int value;

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x9);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);
	reg32_write(DDRP_APBONLY0_MICRORESET, 0x0);

	do {
		while ((reg32_read(DDRP_APBONLY0_UCTSHADOWREGS) & 0x1));

		value = reg32_read(DDRP_APBONLY0_UCTWRITEONLYSHADOW);

		reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x0);

		while (!(reg32_read(DDRP_APBONLY0_UCTSHADOWREGS) & 0x1));

		reg32_write(DDRP_APBONLY0_DCTWRITEPROT, 0x1);
	} while (value != 0x7);

	reg32_write(DDRP_APBONLY0_MICRORESET, 0x1);
}
#endif /* LPDDR4_DEBUG */

static void lpddr4_cfg_pie(struct dram_cfg_param *pie_cfg, int num)
{
	int i = 0;

	for (i = 0; i < num; i++) {
		reg32_write(pie_cfg->reg, pie_cfg->val);
		pie_cfg++;
	}
}

static unsigned int lpddr4_read_msg(void)
{
	unsigned int cdd_cha_u32, cdd_chb_u32;

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);

	cdd_cha_u32 = reg32_read(DDRP_BASE_ADDR + (0x54015 << 2)) & 0xFFFF;
	cdd_chb_u32 = reg32_read(DDRP_BASE_ADDR + (0x5402f << 2)) & 0xFFFF;

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

	return (((cdd_chb_u32 & 0x7F) << 16) | ((cdd_cha_u32 >> 8) & 0x7F));
}

extern unsigned int g_ddr_rate;

void ddr_init(struct dram_timing_info *dram_timing)
{
	unsigned int value, temp;
	unsigned int cdd_cha, cdd_chb, cdd_ch;
	unsigned int rd2wr_val;
	unsigned int txdqsdly_coarse, txdqsdly_fine, trained_txdqsdly;
	unsigned int tctrl_delay, t_wrdata_delay;
	unsigned int fw_src_laddr = 0;
	unsigned int fw_src_len;
	unsigned int rd_byte;

	dram_pll_init(MHZ(g_ddr_rate));

	reg32_write(X2_PMU_DDRSYS_CTRL, 0x1);
	reg32_write(X2_SYSC_DDRSYS_SW_RSTEN, 0xfffffffe);

	reg32_write(DDRC_PHY_DFI1_ENABLE, 0x1);

	reg32_write(DDRC_DBG1, 0x1);
	reg32_write(DDRC_PWRCTL, 0x1);

	while ((reg32_read(DDRC_STAT) & 0x7));

	/*step2 Configure uMCTL2's registers */
	lpddr4_cfg_umctl2(dram_timing->ddrc_cfg, dram_timing->ddrc_cfg_num);

	reg32_write(X2_SYSC_DDRSYS_SW_RSTEN, 0x0);

	reg32_write(DDRC_DBG1, 0x0);
#ifdef CONFIG_SUPPORT_PALLADIUM
	reg32_write(DDRC_PWRCTL, 0x0);
#else
	reg32_write(DDRC_PWRCTL, 0x120);
#endif /* CONFIG_SUPPORT_PALLADIUM */
	reg32_write(DDRC_SWCTL, 0x0);

	/* DFIMISC.dfi_init_compelete_en to 0 */
	value = reg32_read(DDRC_DFIMISC) & ~(1 << 0);
	reg32_write(DDRC_DFIMISC, value);

	/* DFIMISC.dfi_frequency */
	value = reg32_read(DDRC_DFIMISC) | (1 << 12);
	reg32_write(DDRC_DFIMISC, value);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	lpddr4_cfg_phy(dram_timing);

#ifndef CONFIG_SUPPORT_PALLADIUM
	if (g_dev_ops.proc_start) {
		g_dev_ops.proc_start();
	}

	for (int i = 0; i < 2; i++) {
		reg32_write(DDRP_MASTER0_MEMRESETL, 0x2);
		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);

		if (g_dev_ops.pre_read) {
			g_dev_ops.pre_read(&g_binfo, i, 0x0, &fw_src_laddr, &fw_src_len);
		} else {
			fw_src_len = IMEM_LEN;
		}

#if defined(CONFIG_X2_YMODEM_BOOT) || defined(CONFIG_X2_AP_BOOT)
		printf("\nLoad fw imem %dD ...\n", i + 1);
#endif /* CONFIG_X2_YMODEM_BOOT */

		/* Load 32KB firmware */
		rd_byte = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, fw_src_len);

		lpddr4_load_fw(DDRP_BASE_ADDR + IMEM_OFFSET_ADDR,
			X2_SRAM_LOAD_ADDR, rd_byte, IMEM_LEN);

		if (g_dev_ops.post_read) {
			g_dev_ops.post_read(0x0);
		}

		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);
		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);

		if (g_dev_ops.pre_read) {
			g_dev_ops.pre_read(&g_binfo, i, 0x8000, &fw_src_laddr, &fw_src_len);
		} else {
			fw_src_len = DMEM_LEN;
		}

#if defined(CONFIG_X2_YMODEM_BOOT) || defined(CONFIG_X2_AP_BOOT)
		printf("\nLoad fw dmem %dD ...\n", i + 1);
#endif /* CONFIG_X2_YMODEM_BOOT */

		/* Load 16KB firmware */
		rd_byte = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, fw_src_len);

		lpddr4_load_fw(DDRP_BASE_ADDR + DMEM_OFFSET_ADDR,
			X2_SRAM_LOAD_ADDR, rd_byte, DMEM_LEN);

		if (g_dev_ops.post_read) {
			g_dev_ops.post_read(0x0);
		}

		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

		lpddr4_exec_fw();
	}
#endif /* CONFIG_SUPPORT_PALLADIUM */

	cdd_ch = lpddr4_read_msg();

	lpddr4_cfg_pie(dram_timing->ddrphy_pie, dram_timing->ddrphy_pie_num);

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);
	reg32_write(DDRP_MASTER0_PPTTRAINSETUP_P0, 0x6a);
	reg32_write(DDRP_MASTER0_PMIENABLE, 0x1);

	reg32_write(DDRC_SWCTL, 0x0);

	value = reg32_read(DDRC_DFIMISC) | 0x20;
	reg32_write(DDRC_DFIMISC, value);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	while (!(reg32_read(DDRC_DFISTAT) & 0x1));

	reg32_write(DDRC_SWCTL, 0x0);

	value = reg32_read(DDRC_DFIMISC) & ~0x20;
	reg32_write(DDRC_DFIMISC, value);

#ifndef CONFIG_SUPPORT_PALLADIUM
	reg32_write(DDRC_DBG1, 0x1);

	cdd_cha = cdd_ch & 0xFFFF;
	cdd_chb = (cdd_ch >> 16) & 0xFFFF;

	temp = reg32_read(DDRC_DRAMTMG2);
	rd2wr_val = (temp & 0x3F00) >> 8;
	value = ((max(cdd_cha, cdd_chb) + 1) >> 1) + rd2wr_val;
	reg32_write(DDRC_DRAMTMG2, (value <<8) | (temp & 0xFFFFFC0FF));

	temp = reg32_read(DDRP_DBYTE0_TXDQSDLYTG0_U1_P0);
	txdqsdly_coarse = (temp & 0x3C0) >> 6;
	txdqsdly_fine = temp & 0x1F;

	trained_txdqsdly = ((txdqsdly_coarse << 5) + txdqsdly_fine + 0x3F) >> 6;

	temp = reg32_read(DDRC_DFITMG0);
	tctrl_delay = (temp & 0x1F000000) >> 24;

	if (((temp & 0x8000) >> 15) == 1) {
		t_wrdata_delay = (tctrl_delay << 1) + 6 + 8 + trained_txdqsdly + 0x1;
	} else {
		t_wrdata_delay = (tctrl_delay << 1) + 6 + 8 + trained_txdqsdly;
	}

	temp = reg32_read(DDRC_DFITMG1);
	value = ((t_wrdata_delay + 1) >>1 ) << 16;
	value |= (temp & 0xFFE0FFFF);
	reg32_write(DDRC_DFITMG1, value);

	reg32_write(DDRC_DBG1, 0x0);
#endif /* CONFIG_SUPPORT_PALLADIUM */

	value = reg32_read(DDRC_DFIMISC) | 0x1;
	reg32_write(DDRC_DFIMISC, value);

	value = reg32_read(DDRC_PWRCTL) & ~0x20;
	reg32_write(DDRC_PWRCTL, value);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	while (!(reg32_read(DDRC_STAT) & 0x7));

	value = reg32_read(DDRC_RFSHCTL3) & ~0x1;
	reg32_write(DDRC_RFSHCTL3, value);

	reg32_write(DDRC_PWRCTL, 0x0);

	reg32_write(DDRC_PCTRL_0, 0x1);
	reg32_write(DDRC_PCTRL_1, 0x1);
	reg32_write(DDRC_PCTRL_2, 0x1);
	reg32_write(DDRC_PCTRL_3, 0x1);
	reg32_write(DDRC_PCTRL_4, 0x1);
	reg32_write(DDRC_PCTRL_5, 0x1);

#ifndef CONFIG_SUPPORT_PALLADIUM
	if (g_dev_ops.proc_end) {
		g_dev_ops.proc_end(&g_binfo);
	}
#endif /* CONFIG_SUPPORT_PALLADIUM */
}

