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
#include <asm/arch-x2/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch-x2/ddr.h>
#include <hb_info.h>
#include <scomp.h>

DECLARE_GLOBAL_DATA_PTR;
uint32_t x3_ddr_part_num = 0xffffffff;
phys_size_t sys_sdram_size = 0x80000000; /* 2G */
uint32_t hb_board_id = 1;
bool recovery_sys_enable = true;
#define MHZ(x) ((x) * 1000000UL)

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

void hb_set_serial_number(void)
{
	char serial_number[32] = {0, };
	char *env_serial = env_get("serial#");;
	struct mmc *mmc = find_mmc_device(0);

	if(!mmc)
		return;

	snprintf(serial_number, 32, "0x%04x%04x", mmc->cid[2] & 0xffff,
				(mmc->cid[3] >> 16) & 0xffff);
	if (env_serial) {
		if(!strcmp(serial_number, env_serial))
			return;
		run_command("env delete -f serial#", 0);
	}

	env_set("serial#", serial_number);
}

int hb_check_secure(void) {
	uint32_t reg;
	char *if_secure_env = env_get("secure_en");
	int ret = 0;

	reg = reg32_read(X2_GPIO_BASE + X2_STRAP_PIN_REG);
	ret |= PIN_SECURE_SEL(reg);
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
                        reason = "WTD_RESET";
                        break;
                default:
                        break;
        }
        return reason;
}

int dram_init(void)
{
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;
	unsigned int boardid = bootinfo->board_id;
	phys_size_t ddr_size = (boardid >> 16) & 0xf;
	unsigned int ddr_ecc = ECC_CONFIG_SEL(boardid);

	if (ddr_size == 0)
		ddr_size = 1;

	sys_sdram_size = ddr_size * 1024 * 1024 * 1024;

	if (ddr_ecc)
		sys_sdram_size = (sys_sdram_size * 7) / 8;

	DEBUG_LOG("system DDR size: 0x%llx\n", sys_sdram_size - CONFIG_SYS_SDRAM_BASE);

	gd->ram_size = sys_sdram_size - CONFIG_SYS_SDRAM_BASE;
	x3_mem_map[0].size = get_effective_memsize();

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

	if (ddr_ecc == 1)
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
	uint32_t base_board_id = 0;
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;

	hb_board_id = bootinfo->board_id;
	/* work around solution for xj3 bring up ethernet,
	 * all io to v1.8 except bt1120
	 * BIFSPI and I2C2 is 3.3v in J3DVB, the other is 1.8v
	 */
	value = 0xF0F;
	base_board_id = hb_base_board_type_get();
	if (base_board_id == BASE_BOARD_J3_DVB) {
		value = 0xD0D;
	}
	writel(value, GPIO_BASE + 0x174);
	writel(0xF, GPIO_BASE + 0x170);
	return 0;
}

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
