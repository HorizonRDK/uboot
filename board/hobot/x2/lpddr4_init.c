/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#include <common.h>
#include <errno.h>
#include <asm/arch/ddr.h>
#include <asm/arch/ddr_phy.h>
#include <asm/arch/clock.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/hb_share.h>
#include <asm/arch/hb_dev.h>

#if defined(HB_MMC_BOOT) || defined(HB_NOR_BOOT) \
				|| defined(HB_NAND_BOOT)
struct dram_cfg_param lpddr4_ddrc_mono[] = {
	{ DDRP_DBYTE0_DQ0LNSEL, 0x0 },
	{ DDRP_DBYTE0_DQ1LNSEL, 0x1 },
	{ DDRP_DBYTE0_DQ2LNSEL, 0x4 },
	{ DDRP_DBYTE0_DQ3LNSEL, 0x2 },
	{ DDRP_DBYTE0_DQ4LNSEL, 0x7 },
	{ DDRP_DBYTE0_DQ5LNSEL, 0x5 },
	{ DDRP_DBYTE0_DQ6LNSEL, 0x6 },
	{ DDRP_DBYTE0_DQ7LNSEL, 0x3 },

	{ DDRP_DBYTE1_DQ0LNSEL, 0x1 },
	{ DDRP_DBYTE1_DQ1LNSEL, 0x7 },
	{ DDRP_DBYTE1_DQ2LNSEL, 0x5 },
	{ DDRP_DBYTE1_DQ3LNSEL, 0x4 },
	{ DDRP_DBYTE1_DQ4LNSEL, 0x2 },
	{ DDRP_DBYTE1_DQ5LNSEL, 0x0 },
	{ DDRP_DBYTE1_DQ6LNSEL, 0x6 },
	{ DDRP_DBYTE1_DQ7LNSEL, 0x3 },

	{ DDRP_DBYTE2_DQ0LNSEL, 0x3 },
	{ DDRP_DBYTE2_DQ1LNSEL, 0x6 },
	{ DDRP_DBYTE2_DQ2LNSEL, 0x0 },
	{ DDRP_DBYTE2_DQ3LNSEL, 0x1 },
	{ DDRP_DBYTE2_DQ4LNSEL, 0x7 },
	{ DDRP_DBYTE2_DQ5LNSEL, 0x5 },
	{ DDRP_DBYTE2_DQ6LNSEL, 0x2 },
	{ DDRP_DBYTE2_DQ7LNSEL, 0x4 },

	{ DDRP_DBYTE3_DQ0LNSEL, 0x7 },
	{ DDRP_DBYTE3_DQ1LNSEL, 0x0 },
	{ DDRP_DBYTE3_DQ2LNSEL, 0x4 },
	{ DDRP_DBYTE3_DQ3LNSEL, 0x2 },
	{ DDRP_DBYTE3_DQ4LNSEL, 0x5 },
	{ DDRP_DBYTE3_DQ5LNSEL, 0x3 },
	{ DDRP_DBYTE3_DQ6LNSEL, 0x1 },
	{ DDRP_DBYTE3_DQ7LNSEL, 0x6 },
};

struct dram_cfg_param lpddr4_ddrc_som[] = {
	{ DDRP_DBYTE0_DQ0LNSEL, 0x0 },
	{ DDRP_DBYTE0_DQ1LNSEL, 0x1 },
	{ DDRP_DBYTE0_DQ2LNSEL, 0x4 },
	{ DDRP_DBYTE0_DQ3LNSEL, 0x2 },
	{ DDRP_DBYTE0_DQ4LNSEL, 0x7 },
	{ DDRP_DBYTE0_DQ5LNSEL, 0x5 },
	{ DDRP_DBYTE0_DQ6LNSEL, 0x6 },
	{ DDRP_DBYTE0_DQ7LNSEL, 0x3 },

	{ DDRP_DBYTE1_DQ0LNSEL, 0x1 },
	{ DDRP_DBYTE1_DQ1LNSEL, 0x6 },
	{ DDRP_DBYTE1_DQ2LNSEL, 0x5 },
	{ DDRP_DBYTE1_DQ3LNSEL, 0x4 },
	{ DDRP_DBYTE1_DQ4LNSEL, 0x2 },
	{ DDRP_DBYTE1_DQ5LNSEL, 0x0 },
	{ DDRP_DBYTE1_DQ6LNSEL, 0x7 },
	{ DDRP_DBYTE1_DQ7LNSEL, 0x3 },

	{ DDRP_DBYTE2_DQ0LNSEL, 0x3 },
	{ DDRP_DBYTE2_DQ1LNSEL, 0x6 },
	{ DDRP_DBYTE2_DQ2LNSEL, 0x0 },
	{ DDRP_DBYTE2_DQ3LNSEL, 0x1 },
	{ DDRP_DBYTE2_DQ4LNSEL, 0x7 },
	{ DDRP_DBYTE2_DQ5LNSEL, 0x5 },
	{ DDRP_DBYTE2_DQ6LNSEL, 0x2 },
	{ DDRP_DBYTE2_DQ7LNSEL, 0x4 },

	{ DDRP_DBYTE3_DQ0LNSEL, 0x7 },
	{ DDRP_DBYTE3_DQ1LNSEL, 0x0 },
	{ DDRP_DBYTE3_DQ2LNSEL, 0x4 },
	{ DDRP_DBYTE3_DQ3LNSEL, 0x2 },
	{ DDRP_DBYTE3_DQ4LNSEL, 0x5 },
	{ DDRP_DBYTE3_DQ5LNSEL, 0x3 },
	{ DDRP_DBYTE3_DQ6LNSEL, 0x1 },
	{ DDRP_DBYTE3_DQ7LNSEL, 0x6 },
};

static unsigned int x2_board_id[] = {
	0x100, 0x200, 0x201, 0x102, 0x103, 0x101, 0x202, 0x203, 0x204, 0x205,
	0x300, 0x301, 0x302, 0x303, 0x304, 0x400, 0x401, 0x104, 0x105, 0x106
};

#if defined(CONFIG_SPL_GPIO_ID)
static unsigned int x2_gpio_id[] = {
	0xff, 0xff, 0x30, 0x20, 0x10, 0x00, 0x3C, 0xff, 0xff, 0xff,
	0xff, 0x34, 0x36, 0x35, 0x37, 0xff, 0xff, 0xff, 0xff, 0xff
};
#endif
#endif

#if 0
#define LPDDR4_DEBUG	1
#endif /* #if 0 */

#define IMEM_LEN 32768 /* byte */
#define DMEM_LEN 16384 /* byte */

#define IMEM_OFFSET_ADDR 	(0x00050000 * 4)
#define DMEM_OFFSET_ADDR 	(0x00054000 * 4)

extern struct hb_info_hdr g_binfo;

static void lpddr4_cfg_umctl2(struct dram_cfg_param *ddrc_cfg, int num)
{
	int i = 0;

	for (i = 0; i < num; i++) {
		reg32_write(ddrc_cfg->reg, ddrc_cfg->val);
		ddrc_cfg++;
	}
}

#if defined(HB_MMC_BOOT) || defined(HB_NOR_BOOT) \
				|| defined(HB_NAND_BOOT)

static int spl_board_id_verify(unsigned int board_id)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(x2_board_id); i++) {
		if (board_id == x2_board_id[i])
			return 0;
	}

	return -1;
}

#if defined(CONFIG_SPL_GPIO_ID)
static unsigned int spl_gpio_get(void)
{
	unsigned int reg_x, reg_y;

	reg_x = reg32_read(X2_GPIO_BASE + GPIO_GRP5_REG);
	reg_y = reg32_read(X2_GPIO_BASE + GPIO_GRP4_REG);

	return PIN_BOARD_SEL(reg_x, reg_y);
}

static unsigned int spl_gpio_to_board_id(unsigned int gpio_id)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(x2_gpio_id); i++) {
		if (gpio_id == x2_gpio_id[i])
			return x2_board_id[i];
	}

	if (gpio_id == 0x3f)
		return X2_SOM_3V3_ID;

	return 0xff;
}
#endif
#endif

static void lpddr4_cfg_phy(struct dram_timing_info *dram_timing)
{
	int i = 0;
#if defined(CONFIG_SPL_GPIO_ID)
	unsigned int gpio_id = 0;
#endif
#if defined(HB_MMC_BOOT) || defined(HB_NOR_BOOT) \
				|| defined(HB_NAND_BOOT)
	unsigned int size = 0;
	unsigned int board_id = g_binfo.board_id;
#endif
	struct dram_cfg_param *ddrp_cfg = dram_timing->ddrphy_cfg;

	for (i = 0; i < dram_timing->ddrphy_cfg_num; i++) {
		reg32_write(ddrp_cfg->reg, ddrp_cfg->val);
		ddrp_cfg++;
	}
#if defined(HB_MMC_BOOT) || defined(HB_NOR_BOOT) \
				|| defined(HB_NAND_BOOT)

#if defined(CONFIG_SPL_GPIO_ID)
	gpio_id = spl_gpio_get();
	printf("gpio_id: %02x \n", gpio_id);

	if (board_id == HB_GPIO_MODE) {
		gpio_id = spl_gpio_get();

		board_id = spl_gpio_to_board_id(gpio_id);

		if (board_id == 0xff) {
			printf("error: gpio id %02x not support \n", gpio_id);
                        printf("default using DDR SOM configuration !\n");
                        board_id = 0x103;
		}
	} else {
		if (spl_board_id_verify(board_id) != 0) {
		printf("error: board id %02x not support, using DDR"
			" SOM configuration \n", board_id);
                        board_id = 0x103;
		}
	}
#else
	if (spl_board_id_verify(board_id) != 0) {
		printf("error: board id %02x not support, using DDR"
			" SOM configuration \n", board_id);
                board_id = 0x103;
	}
#endif

	switch (board_id) {
	case X2_SVB_BOARD_ID:
	case J2_SVB_BOARD_ID:
		ddrp_cfg = NULL;
		size = 0;
		break;

	case X2_MONO_BOARD_ID:
	case QUAD_BOARD_ID:
		ddrp_cfg = lpddr4_ddrc_mono;
		size = ARRAY_SIZE(lpddr4_ddrc_mono);
		break;

	default:
		ddrp_cfg = lpddr4_ddrc_som;
		size = ARRAY_SIZE(lpddr4_ddrc_som);
		break;
	}

	for (i = 0; i < size; i++) {
		reg32_write(ddrp_cfg->reg, ddrp_cfg->val);
		ddrp_cfg++;
	}
#endif
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

#ifdef CONFIG_X2_PM
#if 0
static void lpddr4_enter_retention(void)
{
	unsigned int val;

	reg32_write(DDRC_PCTRL_0, 0x0);
	reg32_write(DDRC_PCTRL_1, 0x0);
	reg32_write(DDRC_PCTRL_2, 0x0);
	reg32_write(DDRC_PCTRL_3, 0x0);
	reg32_write(DDRC_PCTRL_4, 0x0);
	reg32_write(DDRC_PCTRL_5, 0x0);

	while (reg32_read(DDRC_PSTAT));

	val = reg32_read(DDRC_DERATEEN) & 0xFFFFFFFE;
	reg32_write(DDRC_DERATEEN, val);

	reg32_write(DDRC_DBG1, 0x2);

	while (!(reg32_read(DDRC_DBGCAM) & 0x60000000));

	val = reg32_read(DDRC_DFILPCFG0) & 0xFFFEFEFE;
	reg32_write(DDRC_DFILPCFG0, val);

	val = reg32_read(DDRC_DFILPCFG1) & 0xFFFFFFFE;
	reg32_write(DDRC_DFILPCFG1, val);

	while ((reg32_read(DDRC_DFISTAT) & 0x2));

	val = (reg32_read(DDRC_DFIPHYMSTR) & 0xFFFFFFFE);
	reg32_write(DDRC_DFIPHYMSTR, val);

	val = reg32_read(DDRC_PWRCTL) | 0x20;
	reg32_write(DDRC_PWRCTL, val);

	while ((reg32_read(DDRC_STAT) & 0x7) != 0x3)

	reg32_write(DDRC_SWCTL, 0x0);

	val = reg32_read(DDRC_DFIMISC) & 0xFFFFFFFE;
	reg32_write(DDRC_DFIMISC, val);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	reg32_write(DDRC_SWCTL, 0x0);

	val = reg32_read(DDRC_DFIMISC) | 0x1F20;
	reg32_write(DDRC_DFIMISC, val);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);
	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x3);

	while (!(reg32_read(DDRP_INITENG0_PHYINLP3) & 0x1));

	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x2);
	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

	reg32_write(HB_PMU_DDRSYS_CTRL, 0x0);
	reg32_write(HB_DDRSYS_CLKEN_CLR, 0x1);

	return;
}
#endif
static int lpddr4_save_param(struct dram_timing_info *dram_timing)
{
	uint32_t *pcfg = (uint32_t *)X2_SRAM_LOAD_ADDR;
	uint32_t *pcfg32_data = pcfg;
	uint16_t *pcfg16_data = (uint16_t *)(X2_SRAM_LOAD_ADDR + 8);
	uint32_t *ptable = dram_timing->ddrphy_cali_table;
	uint32_t cfg_size, wr_size = 0;
	int i;

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);
	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x3);

	pcfg32_data[0] = reg32_read(ptable[0]);
	pcfg32_data[1] = reg32_read(ptable[1]);
	for (i = 2; i < dram_timing->ddrphy_cali_num; i++) {
		pcfg16_data[i - 2] = (uint16_t)(reg32_read(ptable[i]) & 0xFFFF);
		//printf("R:0x%x -> 0x%x\n", ptable[i], pcfg16_data[i - 2]);
	}
	cfg_size = (dram_timing->ddrphy_cali_num - 2 )* sizeof(uint16_t) + 8;

	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x2);
	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

	if (g_dev_ops.erase) {
		g_dev_ops.erase(g_binfo.ddrp_addr[0], cfg_size);
	}

	if (g_dev_ops.write) {
		wr_size = g_dev_ops.write(g_binfo.ddrp_addr[0], (uintptr_t)pcfg, cfg_size);
		reg32_write(HB_PMU_SW_REG_00, wr_size);

		printf("Write params(0x%x) to Flash\n", wr_size);
	}

	return 0;
}

static int lpddr4_restore_param(struct dram_timing_info *dram_timing)
{
	uint32_t *pcfg = (uint32_t *)(X2_SRAM_LOAD_ADDR);
	uint32_t *pcfg32_data = pcfg;
	uint16_t *pcfg16_data = (uint16_t *)(X2_SRAM_LOAD_ADDR + 8);
	uint32_t *ptable = dram_timing->ddrphy_cali_table;
	uint32_t cfg_size;
	unsigned int rd_bytes;
	int i;

	if (!g_dev_ops.read) {
		return -1;
	}

	cfg_size = reg32_read(HB_PMU_SW_REG_00);

	rd_bytes = g_dev_ops.read(g_binfo.ddrp_addr[0], (uintptr_t)pcfg, cfg_size);

	printf("Read params(0x%x) from Flash\n", rd_bytes);

	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);
	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x3);

	reg32_write(ptable[0], pcfg32_data[0]);
	reg32_write(ptable[1], pcfg32_data[1]);
	for (i = 2; i < dram_timing->ddrphy_cali_num; i++) {
		reg32_write(ptable[i], pcfg16_data[i - 2]);
		//printf("W:0x%x <- 0x%x\n", ptable[i], pcfg16_data[i - 2]);
	}

	reg32_write(DDRP_DRTUB0_UCCLKHCLKENABLES, 0x2);
	reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

	return 0;
}

#if 0
static inline void disable_cnn_core(void)
{
	u32 reg;

	/* Disable clock of CNN */
	writel(0x33, HB_CNNSYS_CLKEN_CLR);
	while (!((reg = readl(HB_CNNSYS_CLKOFF_STA)) & 0xF));
	udelay(5);

	reg = readl(HB_PMU_VDD_CNN_CTRL) | 0x22;
	writel(reg, HB_PMU_VDD_CNN_CTRL);
	udelay(5);

	writel(0x3, HB_SYSC_CNNSYS_SW_RSTEN);
	udelay(5);

	reg = readl(HB_PMU_VDD_CNN_CTRL) & ~0x11;
	writel(reg, HB_PMU_VDD_CNN_CTRL);

	printf("Disable cnn cores ...\n");
}

static inline void enable_cnn_core(void)
{
	u32 reg;

	reg = readl(HB_PMU_VDD_CNN_CTRL) | 0x11;
	writel(reg, HB_PMU_VDD_CNN_CTRL);
	mdelay(2);

	reg = readl(HB_PMU_VDD_CNN_CTRL) & ~0x22;
	writel(reg, HB_PMU_VDD_CNN_CTRL);
	udelay(5);

	writel(0x0, HB_SYSC_CNNSYS_SW_RSTEN);
	udelay(5);

	writel(0x33, HB_CNNSYS_CLKEN_SET);
	udelay(5);

	printf("Enable cnn cores ...\n");
}

static void do_core1_idle(void)
{
	wfi();
}

static inline void do_suspend(void)
{
	void (*core1_func)(void) = &do_core1_idle;
	u32 reg = readl(0xA6003070);
	/* Set wakeup_in pin to func0 */
	reg &= ~0xC000;
	writel(reg, 0xA6003070);

	/* Notify core1 to wfi */
	writeq((uintptr_t)core1_func, CPU_RELEASE_ADDR);
	asm volatile ("sev");

#if 0
	/* Config gpio06 to func02 to capture signals. */
	reg = (readl(0xA6003060)) & ~0xFFFF;
	reg |= 0xAAAA;
	writel(reg, 0xA6003060);

	writel(0x80000155, 0xA6003150);
	writel(0x100, 0xA6003154);
#endif /* #if 0 */

	writel(0x80000000, HB_PMU_SLEEP_PERIOD);
	writel(0xFE, HB_PMU_W_SRC_MASK);

	printf("Enter suspend ...\n");

	writel(0x0, HB_PMU_OUTPUT_CTRL);

	writel(0x1, HB_PMU_SLEEP_CMD);

	wfi();
}
#endif
#endif /* CONFIG_X2_PM */

static inline void lpddr4_cfg_fw(struct dram_timing_info *dram_timing,
	unsigned int wake_src)
{
	unsigned int fw_src_laddr = 0;
	unsigned int fw_src_len;
	unsigned int rd_byte;

	unsigned int mcu_sram;
	unsigned int min_len;

	if (wake_src > 0) {
		return;
	}

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

#if defined(CONFIG_HB_YMODEM_BOOT) || defined(CONFIG_HB_AP_BOOT)
		printf("\nLoad fw imem %dD ...\n", i + 1);
#endif /* CONFIG_HB_YMODEM_BOOT */

		/* Load 32KB firmware */
		mcu_sram = DDRP_BASE_ADDR + IMEM_OFFSET_ADDR;
		while (fw_src_len > 0) {
			min_len = fw_src_len > X2_SRAM_LOAD_MAX ?
				X2_SRAM_LOAD_MAX : fw_src_len;
			rd_byte = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, min_len);

			lpddr4_load_fw(mcu_sram, X2_SRAM_LOAD_ADDR, rd_byte, rd_byte);

			fw_src_len -= min_len;
			fw_src_laddr += min_len;
			mcu_sram += min_len * 2;
		}

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

#if defined(CONFIG_HB_YMODEM_BOOT) || defined(CONFIG_HB_AP_BOOT)
		printf("\nLoad fw dmem %dD ...\n", i + 1);
#endif /* CONFIG_HB_YMODEM_BOOT */

		/* Load 16KB firmware */
		mcu_sram = DDRP_BASE_ADDR + DMEM_OFFSET_ADDR;
		while (fw_src_len > 0) {
			min_len = fw_src_len > X2_SRAM_LOAD_MAX ?
				X2_SRAM_LOAD_MAX : fw_src_len;
			rd_byte = g_dev_ops.read(fw_src_laddr, X2_SRAM_LOAD_ADDR, min_len);

			lpddr4_load_fw(mcu_sram, X2_SRAM_LOAD_ADDR, rd_byte, rd_byte);

			fw_src_len -= min_len;
			fw_src_laddr += min_len;
			mcu_sram += min_len * 2;
		}

		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x1);

		lpddr4_exec_fw();

		if (g_dev_ops.post_read) {
			g_dev_ops.post_read(0x0);
		}
	}
#endif /* CONFIG_SUPPORT_PALLADIUM */

	return;
}

extern unsigned int g_ddr_rate;

void ddr_init(struct dram_timing_info *dram_timing)
{
	unsigned int value, temp;
	unsigned int cdd_cha, cdd_chb, cdd_ch = 0;
	unsigned int rd2wr_val;
	unsigned int txdqsdly_coarse, txdqsdly_fine, trained_txdqsdly;
	unsigned int tctrl_delay, t_wrdata_delay;
	unsigned int wk_sta;
	unsigned int resume_addr;

	dram_pll_init(MHZ(g_ddr_rate));

	reg32_write(HB_PMU_DDRSYS_CTRL, 0x1);
	reg32_write(HB_SYSC_DDRSYS_SW_RSTEN, 0xfffffffe);

	reg32_write(DDRC_PHY_DFI1_ENABLE, 0x1);

	reg32_write(DDRC_DBG1, 0x1);
	reg32_write(DDRC_PWRCTL, 0x1);

	while ((reg32_read(DDRC_STAT) & 0x7));

	/*step2 Configure uMCTL2's registers */
	lpddr4_cfg_umctl2(dram_timing->ddrc_cfg, dram_timing->ddrc_cfg_num);

	reg32_write(HB_SYSC_DDRSYS_SW_RSTEN, 0x0);

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

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	lpddr4_cfg_phy(dram_timing);

	wk_sta = readl(HB_PMU_WAKEUP_STA);
	printf("wake sta = 0x%x, src = 0x%x\n", wk_sta, readl(HB_PMU_W_SRC));

	resume_addr = readl(HB_PMU_SW_REG_03);

	if (wk_sta != 0 && resume_addr == 0)
		wk_sta = 0;

	lpddr4_cfg_fw(dram_timing, wk_sta);

	if (wk_sta == 0) {
		cdd_ch = lpddr4_read_msg();
	} else {
#ifdef CONFIG_X2_PM
//		printf("Exit from suspend ...\n");
		lpddr4_restore_param(dram_timing);

//		enable_cnn_core();

//		printf("End of restore ...\n");
#endif /* CONFIG_X2_PM */
	}

	lpddr4_cfg_pie(dram_timing->ddrphy_pie, dram_timing->ddrphy_pie_num);

	if (wk_sta == 0) {
		reg32_write(DDRP_APBONLY0_MICROCONTMUXSEL, 0x0);
		reg32_write(DDRP_MASTER0_PPTTRAINSETUP_P0, 0x6a);
		reg32_write(DDRP_MASTER0_PMIENABLE, 0x1);
	}

	reg32_write(DDRC_SWCTL, 0x0);

	value = reg32_read(DDRC_DFIMISC) | 0x20;
	reg32_write(DDRC_DFIMISC, value);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	while (!(reg32_read(DDRC_DFISTAT) & 0x1));

	reg32_write(DDRC_SWCTL, 0x0);

	value = reg32_read(DDRC_DFIMISC) & ~0x20;
	reg32_write(DDRC_DFIMISC, value);

	if (wk_sta == 0) {
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

		trained_txdqsdly = ((txdqsdly_coarse << 5) +
			txdqsdly_fine + 0x3F) >> 6;

		temp = reg32_read(DDRC_DFITMG0);
		tctrl_delay = (temp & 0x1F000000) >> 24;

		t_wrdata_delay = (tctrl_delay << 1) + 14 + trained_txdqsdly;
		if (((temp & 0x8000) >> 15) == 1) {
			t_wrdata_delay += 0x1;
		}

		temp = reg32_read(DDRC_DFITMG1);
		value = ((t_wrdata_delay + 1) >>1 ) << 16;
		value |= (temp & 0xFFE0FFFF);
		reg32_write(DDRC_DFITMG1, value);

		reg32_write(DDRC_DBG1, 0x0);
#endif /* CONFIG_SUPPORT_PALLADIUM */
	}

	value = reg32_read(DDRC_DFIMISC) | 0x1;
	reg32_write(DDRC_DFIMISC, value);

	reg32_write(DDRC_SWCTL, 0x1);

	while (!(reg32_read(DDRC_SWSTAT) & 0x1));

	value = reg32_read(DDRC_PWRCTL) & ~0x20;
	reg32_write(DDRC_PWRCTL, value);

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

	if (wk_sta == 0) {
#ifndef CONFIG_SUPPORT_PALLADIUM
		if (g_dev_ops.proc_end) {
			g_dev_ops.proc_end(&g_binfo);
		}
#endif /* CONFIG_SUPPORT_PALLADIUM */

#ifdef CONFIG_X2_PM
		lpddr4_save_param(dram_timing);

#if 0
		disable_cnn_core();

		printf("Enter self-refresh ...\n");
		lpddr4_enter_retention();

		do_suspend();
#endif
	} else if (wk_sta == 2 || wk_sta == 1) {
		void (*x2_resume)(void);
		x2_resume = (void(*)(void))resume_addr;

		writel(0, HB_PMU_SW_REG_03);

		invalidate_icache_all();
		x2_resume();
#endif /* CONFIG_X2_PM */
       }
}
