/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#include <common.h>
#include <sata.h>
#include <ahci.h>
#include <scsi.h>
#include <mmc.h>
#include <malloc.h>
#include <asm/io.h>

#include <asm/armv8/mmu.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch-x2/ddr.h>
#include <hb_info.h>
#include <scomp.h>
#include "qos_hex.h"

DECLARE_GLOBAL_DATA_PTR;
uint32_t x3_ddr_part_num = 0xffffffff;
phys_size_t sys_sdram_size = 0x80000000; /* 2G */
uint32_t hb_board_id = 1;
bool recovery_sys_enable = true;
extern struct hb_uid_hdr hb_unique_id;

#define MHZ(x) ((x) * 1000000UL)
#define DEBUG_SECURE_BOOT_PIN  0

static void hb_unique_id_get(void)
{
    struct hb_uid_hdr *p_uid = (struct hb_uid_hdr *)(HB_UNIQUEID_INFO);
    hb_unique_id = *p_uid;
    return;
}

int get_pin_input_value(char pin)
{
	unsigned int reg = 0;
	unsigned int offset = 0;
	unsigned int value = 0;

	if (pin <= 0 || pin > HB_PIN_MAX_NUMS) {
		printf("set pin is error\n");
		return 0;
	}

	/* set pin to gpio*/
	offset = pin * 4;
	reg = reg32_read(X2A_PIN_SW_BASE + offset);
	reg |= 3;
	reg32_write(X2A_PIN_SW_BASE + offset, reg);

	/* set pin to input */
	offset = (pin / 16) * 0x10 + 0x08;
	reg = reg32_read(X2_GPIO_BASE + offset);
	reg &= (~(1 << ((pin % 16) + 16)));
	reg32_write(X2_GPIO_BASE + offset, reg);

	/* get input value */
	offset = (pin / 16) * 0x10 + 0x0c;
	value = reg32_read(X2_GPIO_BASE + offset);
	value = (value >>(pin %16)) & 0x01;
	return value;
}

/* Update Peri PLL */
void switch_sys_pll(ulong pll_val)
{
	unsigned int value;
	unsigned int try_num = 5;

	value = readl(HB_PLLCLK_SEL) & (~SYSCLK_SEL_BIT);
	writel(value, HB_PLLCLK_SEL);

	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
		HB_SYSPLL_PD_CTRL);

	switch (pll_val) {
		case MHZ(1200):
			value = FBDIV_BITS(100) | REFDIV_BITS(1) |
				POSTDIV1_BITS(2) | POSTDIV2_BITS(1);
			break;
		case MHZ(1500):
		default:
			value = FBDIV_BITS(125) | REFDIV_BITS(1) |
				POSTDIV1_BITS(2) | POSTDIV2_BITS(1);
			break;
	}

	writel(value, HB_SYSPLL_FREQ_CTRL);

	value = readl(HB_SYSPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_SYSPLL_PD_CTRL);

	while (!(value = readl(HB_SYSPLL_STATUS) & LOCK_BIT)) {
		if (try_num <= 0) {
			break;
		}

		udelay(100);
		try_num--;
	}

	value = readl(HB_PLLCLK_SEL);
	value |= SYSCLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	return;
}

static struct mm_region x3_mem_map[] = {
	{
		/* SDRAM space */
		.virt = CONFIG_SYS_SDRAM_BASE,
		.phys = CONFIG_SYS_SDRAM_BASE,
		.size = CONFIG_SYS_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},
	{
		/* SRAM space */
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x200000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},
	{
		/* GIC space */
		.virt = 0x90000000UL,
		.phys = 0x90000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},
	{
		/* SDRAM-2 space */
		.virt = PHYS_SDRAM_2,
		.phys = PHYS_SDRAM_2,
		.size = PHYS_SDRAM_2_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},
	{
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = x3_mem_map;

int hb_boot_mode_get(void) {
	unsigned int reg;

	reg = reg32_read(X2_GPIO_BASE + X2_STRAP_PIN_REG);

	return PIN_2NDBOOT_SEL(reg);
}

#ifdef CONFIG_FASTBOOT
int hb_fastboot_key_pressed(void) {
	unsigned int reg;

	reg = reg32_read(X2_GPIO_BASE + X2_STRAP_PIN_REG);

	return PIN_FASTBOOT_SEL(reg);
}
#endif

#if defined(CONFIG_DFU_OVER_USB) || defined(CONFIG_SET_DFU_ALT_INFO)
void set_dfu_alt_info(char *interface, char *devstr)
{
	unsigned int boot_mode = hb_boot_mode_get();
	char *alt_info = NULL;

	switch (boot_mode) {
	case PIN_2ND_NAND:
		alt_info = DFU_ALT_INFO_SPINAND;
		break;
	case PIN_2ND_EMMC:
	default:
		alt_info = DFU_ALT_INFO_EMMC;
		break;
	}

	if (alt_info)
		env_set("dfu_alt_info", alt_info);

	puts("DFU alt info setting: done\n");
}
#endif

int hb_get_socuid(char *socuid)
{
	int read_flag = 0;
	u32 val;
	int32_t word;
    char tmp[10] = {0};

	for (word = 3; word >= 0; word--) {
        val = hb_unique_id.bank[word];
        if (val != 0) {
            read_flag = 1;
        }
        snprintf(tmp, sizeof(tmp), "%.8x", val);
        snprintf(socuid + strlen(socuid), sizeof(tmp), tmp);
	}
	if (read_flag == 0) {
        snprintf(socuid, sizeof(tmp), "%.8x", 0);
        for (word = 0; word < 3; word++) {
            val = api_efuse_read_data(word + SOCNID_BANK);
            if (val != 0) {
                read_flag = 1;
            }
            snprintf(tmp, sizeof(tmp), "%.8x", val);
            snprintf(socuid + strlen(socuid), sizeof(tmp), tmp);
        }
	}

	return read_flag;
}


uint32_t hb_efuse_chip_type(void)
{
	uint32_t value = 0;
	uint32_t chip_type = 0;
	uint32_t ret = 0;

	/* get efuse value: bank28 */
	value = scomp_read_sw_efuse_bnk(28);

	/* using efuse som type */
	chip_type = CHIP_TYPE_EFUSE_SEL(value);
	if (chip_type > 0 && chip_type < 4) {
		ret = CHIP_TYPE_X3;
	} else {
		ret = CHIP_TYPE_J3;
	}
	return ret;
}

uint32_t is_bpu_clock_limit(void)
{
		uint32_t value = 0;
		/* get efuse value: bank28 */
		value = scomp_read_sw_efuse_bnk(28);
		return BPU_CLK_EFUSE_SEL(value);
}

void hb_set_serial_number(void)
{
	uint32_t serial_src = 0;
	char serial_number[32] = {0, };
	char *env_serial = env_get("serial#");
	char efuse_uid[32] __aligned(4) = { 0 };
	char *efuse_uid_half;
	int efuse_uid_not_zero = hb_get_socuid(efuse_uid);

	/* If socuid present, use it as device identifier and quit */
	if (efuse_uid_not_zero) {
		/* Since Host has limit, use half of it */
		efuse_uid_half = &efuse_uid[16];
		snprintf(serial_number, sizeof(serial_number), "0x%s", efuse_uid_half);
	} else {
		struct mmc *mmc = find_mmc_device(0);
		/* In case of neither socuid nor mmc present, use random number */
		if(!mmc)
			serial_src = rand();
		else
			serial_src = (((uint32_t)(mmc->cid[2] & 0xffff) << 16) | ((mmc->cid[3] >> 16) & 0xffff));

		snprintf(serial_number, sizeof(serial_number), "0x%08x", serial_src);
	}

	if (env_serial) {
		if(!strcmp(serial_number, env_serial))
			return;
		run_command("env delete -f serial#", 0);
	}

	env_set("serial#", serial_number);
}

int hb_check_secure(void) {
#ifdef CONFIG_HB_QUICK_BOOT
	return false;
#endif
	char *if_secure_env = env_get("secure_en");
	int ret = 0;

	if (DEBUG_SECURE_BOOT_PIN > 0  &&
	    DEBUG_SECURE_BOOT_PIN < DEBUG_SECURE_BOOT_PIN) {
		ret = get_pin_input_value(DEBUG_SECURE_BOOT_PIN);
	}
	/*
	 * use "secure_en" to control avb functionality
	 */
	if (if_secure_env) {
		if (!strcmp(if_secure_env, "false"))
			ret = 0;
		else
			ret |= (!strcmp(if_secure_env, "true"));
	}
	ret |= scomp_read_sw_efuse_bnk(22) & 0x8;
	return ret;
}

/* Detect baud rate for console according to bootsel */
unsigned int detect_baud(void)
{
	unsigned int br_sel = hb_pin_get_uart_br();;

	return (br_sel > 0 ? UART_BAUDRATE_115200 : UART_BAUDRATE_921600);
}

char *hb_reset_reason_get()
{
	uint32_t value;
	uint32_t wdt_flag = 0;
	char *reason = NULL;

	value =  readl(HB_PMU_WAKEUP_STA);
        switch (value)
        {
                case 0:
                        reason = "POWER_RESET";
                        break;
                case 1:
                        reason = "SYNC_WAKEUP";
                        break;
                case 2:
                        reason = "ASYNC_WAKEUP";
                        break;
                case 3:
			wdt_flag = 1;
                        break;
                default:
                        break;
        }
	if (wdt_flag) {
		/*Get more detailed reset reasons*/
		value =  (readl(HB_PMU_SW_REG_05) >> HB_RESET_BIT_OFFSET) & 0x0f;
		switch (value) {
			case 0:
				reason = "WTD_RESET";
				break;
			case 1:
				reason = "PANIC_RESET";
				break;
			case 2:
				reason = "NORMAL_RESET";
				break;
			case 3:
				reason = "UBOOT_RESET";
				break;
			default:
				break;
		}
		value = readl(HB_PMU_SW_REG_05) & ~(0xf << HB_RESET_BIT_OFFSET);
		writel(value, HB_PMU_SW_REG_05);
	}
        return reason;
}

int dram_init(void)
{
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;
	unsigned int boardid = bootinfo->board_id;
	phys_size_t ddr_size = (boardid >> 16) & 0xf;
	unsigned int ddr_ecc = ECC_CONFIG_SEL(boardid);
	uint32_t i = 0;

	if (ddr_size == 0)
		ddr_size = 1;

	sys_sdram_size = ddr_size * 1024 * 1024 * 1024;

	if (ddr_ecc)
		sys_sdram_size = (sys_sdram_size * 7) / 8;

	DEBUG_LOG("system DDR size: 0x%llx\n", sys_sdram_size - CONFIG_SYS_SDRAM_BASE);

	gd->ram_size = sys_sdram_size - CONFIG_SYS_SDRAM_BASE;
	x3_mem_map[0].size = get_effective_memsize();
#if defined(CONFIG_NR_DRAM_BANKS) && defined(CONFIG_SYS_SDRAM_BASE)
#if defined(PHYS_SDRAM_2) && defined(CONFIG_MAX_MEM_MAPPED)
	if (ddr_size <= 2) {
		for (i = 0; x3_mem_map[i].size || x3_mem_map[i].attrs; i++) {
			if (x3_mem_map[i].phys >= PHYS_SDRAM_2) {
				memset(&x3_mem_map[i], 0, sizeof(struct mm_region));
			}
		}
	}
#endif  // CONFIG_NR_DRAM_BANKS && CONFIG_SYS_SDRAM_BASE
#endif  // PHYS_SDRAM_2 && CONFIG_MAX_MEM_MAPPED

	hb_board_id = boardid;
	x3_ddr_part_num = bootinfo->reserved[1];
	return 0;
}

int dram_init_banksize(void)
{
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;
	unsigned int ddr_ecc = ECC_CONFIG_SEL(bootinfo->board_id);

#if defined(CONFIG_NR_DRAM_BANKS) && defined(CONFIG_SYS_SDRAM_BASE)
#if defined(PHYS_SDRAM_2) && defined(CONFIG_MAX_MEM_MAPPED)
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_effective_memsize();
	gd->bd->bi_dram[1].start =
		gd->ram_size > (CONFIG_SYS_SDRAM_SIZE + CONFIG_SYS_SDRAM_BASE)
		 ? PHYS_SDRAM_2 : 0;

	if (ddr_ecc)
		gd->bd->bi_dram[1].size =
			gd->ram_size > (CONFIG_SYS_SDRAM_SIZE + CONFIG_SYS_SDRAM_BASE)
			? (sys_sdram_size - PHYS_SDRAM_2_SIZE) : 0;
	else
		gd->bd->bi_dram[1].size =
			gd->ram_size > (CONFIG_SYS_SDRAM_SIZE + CONFIG_SYS_SDRAM_BASE)
			 ? PHYS_SDRAM_2_SIZE: 0;
#else
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_effective_memsize();
#endif
#endif
	return 0;
}

int board_init(void)
{
	switch_sys_pll(MHZ(1500));
	return 0;
}

int timer_init(void)
{
	writel(0x1, PMU_SYSCNT_BASE);
	return 0;
}

int board_late_init(void)
{
#if 0
	qspi_flash_init();
#endif

	hb_unique_id_get();
	hb_set_serial_number();

#ifdef CONFIG_DDR_BOOT
	env_set("bootmode", "ddrboot");
#endif

#ifdef CONFIG_SD_BOOT
	env_set("bootmode", "sdboot");
#endif

#ifdef CONFIG_QSPI_BOOT
	env_set("bootmode", "qspiboot");
#endif

#ifdef CONFIG_USB_ETHER
	usb_ether_init();
#endif

	return 0;
}

unsigned int hb_gpio_get(void)
{
	return 0;
}

unsigned int hb_gpio_to_board_id(unsigned int gpio_id)
{
	return 0;
}

int board_id_verify(unsigned int board_id)
{
	return 0;
}

#ifdef X3_USABLE_RAM_TOP
ulong board_get_usable_ram_top(ulong total_size)
{
	return X3_USABLE_RAM_TOP;
}
#endif

int init_io_vol(void)
{
	uint32_t value = 0;
	uint32_t som_type = 0;
	uint64_t addr = 0, reg = 0;
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;

	hb_board_id = bootinfo->board_id;
	/* work around solution for xj3 bring up ethernet,
	 * all io to v1.8 except bt1120
	 * BIFSPI and I2C2 is 3.3v in J3DVB, the other is 1.8v
	 */
	/*
	 * 1'b0=3.3v mode;  1'b1=1.8v mode
	 * 0x170 bit[3]       sd2
	 *       bit[2]       sd1
	 *       bit[1:0]     sd0
	 *
	 * 0x174 bit[11:10]   rgmii
	 *       bit[9]       i2c2
	 *       bit[8]       i2c0
	 *       bit[7]       reserved
	 *       bit[6:4]     bt1120
	 *       bit[3:2]     bifsd
	 *       bit[1]       bifspi
	 *       bit[0]       jtag
	 */
	som_type = hb_som_type_get();
	if (som_type == SOM_TYPE_J3) {
		writel(0xD0D, GPIO_BASE + 0x174);
		writel(0xF, GPIO_BASE + 0x170);
	} else if (som_type == SOM_TYPE_X3) {
		writel(0xF0F, GPIO_BASE + 0x174);
		writel(0xF, GPIO_BASE + 0x170);
	} else if (som_type == SOM_TYPE_X3SDB) {
		writel(0xF0F, GPIO_BASE + 0x174);
		writel(0x7, GPIO_BASE + 0x170);
	} else if (som_type == SOM_TYPE_X3SDBV4) {
		writel(0xF0F, GPIO_BASE + 0x174);
		writel(0x7, GPIO_BASE + 0x170);
	} else if (som_type == SOM_TYPE_X3PI || som_type == SOM_TYPE_X3PIV2) {
		writel(0xC00, GPIO_BASE + 0x174);
		writel(0x7, GPIO_BASE + 0x170);
		/* Power down and up vdd_sd of sdio2
		 * make sd card run in super speed mode
		 */
		/* Control gpio21 to power down VDD_SD */
		addr = (PIN_MUX_BASE + (21) * 0x4);
		reg = readl(addr);
		reg |= 0x03;
		writel(reg, addr);

		addr = (GPIO_BASE + ((21) / 16) * 0x10 + 0x8);
		reg = readl(addr);
		reg |= ((uint64_t)(0x1) << (16 + 5));
		reg &= ~((uint64_t)(0x1) << (5));
		writel(reg, addr);
		mdelay(10);
		reg |= ((uint64_t)(0x1) << (5));
		writel(reg, addr);
		pr_err("X3 PI reset VDD_SD done\n");
	} else if (som_type == SOM_TYPE_X3E) {
		writel(0xF0F, GPIO_BASE + 0x174);
		writel(0x7, GPIO_BASE + 0x170);
	}
	return 0;
}

void vio_pll_init(void)
{
	unsigned int value;
	unsigned int try_num = 5;

	 /* VIOPLL2: Disable */
	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
			HB_VIOPLL2_PD_CTRL);

	/* VIOPLL2 Freq: 24MHz*99/1/1/1 */
	writel(FBDIV_BITS(99) | REFDIV_BITS(1) | POSTDIV1_BITS(1) | POSTDIV2_BITS(1),
			HB_VIOPLL2_FREQ_CTRL);

	/* VIOPLL2: Enable */
	value = readl(HB_VIOPLL2_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_VIOPLL2_PD_CTRL);

	/* To reconfig VIOPLL should switch MUX to 24MHz becuase the PLL is using */
	value = readl(HB_PLLCLK_SEL);
	value &= ~VIOCLK_SEL_BIT;
	writel(value, HB_PLLCLK_SEL);

	/* VIOPLL: Disable */
	writel(PD_BIT | DSMPD_BIT | FOUTPOST_DIV_BIT | FOUTVCO_BIT,
			HB_VIOPLL_PD_CTRL);

	/* VIOPLL Freq: 24MHz*68/1/1/1 */
	writel(FBDIV_BITS(68) | REFDIV_BITS(1) | POSTDIV1_BITS(1) | POSTDIV2_BITS(1),
			HB_VIOPLL_FREQ_CTRL);

	/* VIOPLL: Enable */
	value = readl(HB_VIOPLL_PD_CTRL);
	value &= ~(PD_BIT | FOUTPOST_DIV_BIT);
	writel(value, HB_VIOPLL_PD_CTRL);

	/* VIOPLL/VIOPLL2 Locked */
	while (!(readl(HB_VIOPLL_STATUS) & LOCK_BIT) ||
		   !(readl(HB_VIOPLL2_STATUS) & LOCK_BIT)) {
		if (try_num <= 0)
			break;

		udelay(100);
		try_num--;
	}
	udelay(500);
	value = readl(HB_PLLCLK_SEL);
	value |= (VIOCLK_SEL_BIT | VIOCLK2_SEL_BIT);
	writel(value, HB_PLLCLK_SEL);

	writel(IAR_PIX_CLK_SRC_SEL(0)                  |
		   SENSOR3_MCLK_2ND_DIV_SEL(REFCLK_DIV(4)) |
		   SENSOR2_MCLK_2ND_DIV_SEL(REFCLK_DIV(4)) |
		   IAR_PIX_CLK_2ND_DIV_SEL(REFCLK_DIV(1))  |
		   IAR_PIX_CLK_1ST_DIV_SEL(REFCLK_DIV(10)) |
		   SIF_MCLK_DIV_SEL(REFCLK_DIV(3))         |
		   SENSOR_DIV_CLK_SRC_SEL(1)               |
		   SENSOR1_MCLK_2ND_DIV_SEL(REFCLK_DIV(4)) |
		   SENSOR0_MCLK_2ND_DIV_SEL(REFCLK_DIV(4)) |
		   SENSOR_MCLK_1ST_DIV_SEL(REFCLK_DIV(16)) ,
		   HB_VIOSYS_CLK_DIV_SEL1);

	writel(MIPI_TX_IPI_CLK_2ND_DIV_SEL(REFCLK_DIV(3)) |
		   MIPI_TX_IPI_CLK_1ST_DIV_SEL(REFCLK_DIV(1)) |
		   MIPI_CFG_CLK_2ND_DIV_SEL(REFCLK_DIV(4))    |
		   MIPI_CFG_CLK_1ST_DIV_SEL(REFCLK_DIV(4))    |
		   PYM_MCLK_SRC_SEL(1)                        |
		   PYM_MCLK_DIV_SEL(REFCLK_DIV(2))            |
		   MIPI_PHY_REFCLK_2ND_DIV_SEL(REFCLK_DIV(4)) |
		   MIPI_PHY_REFCLK_1ST_DIV_SEL(REFCLK_DIV(17)),
		   HB_VIOSYS_CLK_DIV_SEL2);

	writel(MIPI_RX3_IPI_CLK_2ND_DIV_SEL(REFCLK_DIV(3)) |
		   MIPI_RX3_IPI_CLK_1ST_DIV_SEL(REFCLK_DIV(1)) |
		   MIPI_RX2_IPI_CLK_2ND_DIV_SEL(REFCLK_DIV(3)) |
		   MIPI_RX2_IPI_CLK_1ST_DIV_SEL(REFCLK_DIV(1)) |
		   MIPI_RX1_IPI_CLK_2ND_DIV_SEL(REFCLK_DIV(3)) |
		   MIPI_RX1_IPI_CLK_1ST_DIV_SEL(REFCLK_DIV(1)) |
		   MIPI_RX0_IPI_CLK_2ND_DIV_SEL(REFCLK_DIV(3)) |
		   MIPI_RX0_IPI_CLK_1ST_DIV_SEL(REFCLK_DIV(1)) ,
		   HB_VIOSYS_CLK_DIV_SEL3);

	/* X2_VIOSYS_CLKEN_SET default on */

#define IPS_CLK_CTRL		 0xA400000C
#define IPS_CLK_CTRL_SIF     (1 << 0)
#define IPS_CLK_CTRL_ISP0    (1 << 1)
#define IPS_CLK_CTRL_DEW0    (1 << 3)
#define IPS_CLK_CTRL_DEW1    (1 << 4)
#define IPS_CLK_CTRL_GDC0    (1 << 5)
#define IPS_CLK_CTRL_GDC1    (1 << 6)
#define IPS_CLK_CTRL_LDC0    (1 << 9)
#define IPS_CLK_CTRL_LDC1    (1 << 10)
#define IPS_CLK_CTRL_IPU0    (1 << 11)
#define IPS_CLK_CTRL_PYM_UV  (1 << 13)
#define IPS_CLK_CTRL_PYM_US  (1 << 14)
#define IPS_CLK_CTRL_PYM_DDR (1 << 15)
#define IPS_CLK_CTRL_PYM_ISP (1 << 16)
#define IPS_CLK_CTRL_MD      (1 << 17)
#define IPS_CLK_CTRL_IRAM    (1 << 18)

	/* IPS Clock Enable */
	value = readl(IPS_CLK_CTRL);
	value = value                |
			IPS_CLK_CTRL_SIF     |
			IPS_CLK_CTRL_ISP0    |
			IPS_CLK_CTRL_DEW0    |
			IPS_CLK_CTRL_DEW1    |
			IPS_CLK_CTRL_GDC0    |
			IPS_CLK_CTRL_GDC1    |
			IPS_CLK_CTRL_LDC0    |
			IPS_CLK_CTRL_LDC1    |
			IPS_CLK_CTRL_IPU0    |
			IPS_CLK_CTRL_PYM_UV  |
			IPS_CLK_CTRL_PYM_US  |
			IPS_CLK_CTRL_PYM_DDR |
			IPS_CLK_CTRL_PYM_ISP |
			IPS_CLK_CTRL_MD      |
			IPS_CLK_CTRL_IRAM;

	writel(value, IPS_CLK_CTRL);
}

#ifdef SET_QOS_IN_UBOOT
int update_qos(void)
{
#define WRITE_QOS_VALUE   0x0302000c
#define READ_QOS_VALUE    0x0302000c
#define WRITE_QOS_ADDR    0xA2D10004
#define READ_QOS_ADDR     0xA2D10000
#define QOS_BIN_ADDR      0x8000A000
	if (readl(READ_QOS_ADDR) == READ_QOS_VALUE &&
	    readl(WRITE_QOS_ADDR) == WRITE_QOS_VALUE)
		return 0;
	memcpy((void *)QOS_BIN_ADDR, qos_hex, sizeof(qos_hex));
	((void(*)(unsigned int, unsigned int))QOS_BIN_ADDR)(WRITE_QOS_VALUE, READ_QOS_VALUE);
	return 0;
}
#endif

void change_sys_pclk_250M(void)
{
	uint32_t reg = readl(HB_SYS_PCLK);
	reg &= ~SYS_PCLK_DIV_SEL(0x7);
	reg |= SYS_PCLK_DIV_SEL(0x5);
	writel(reg, HB_SYS_PCLK);
}

int hb_get_cpu_num(void)
{
	uint32_t reg = readl(HB_CPU_FLAG);

	if (reg & 0x2)
		return 2;
	else
		return 0;
}
