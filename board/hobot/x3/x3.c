#include <common.h>
#include <sata.h>
#include <ahci.h>
#include <scsi.h>
#include <malloc.h>
#include <asm/io.h>

#include <asm/armv8/mmu.h>
#include <asm/arch/hb_reg.h>
#include <asm/arch/hb_sysctrl.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned int sys_sdram_size = 0x80000000; /* 2G */
bool recovery_sys_enable = true;
unsigned int hb_src_boot = 1;

#define MHZ(x)	((x) * 1000000UL)

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
		.virt = 0x0UL,
		.phys = 0x0UL,
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

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

int board_init(void)
{
	switch_sys_pll(MHZ(1500));
//	switch_sys_pll(MHZ(1200));
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

#ifdef CONFIG_DDR_BOOT
	env_set("bootmode", "ddrboot");
#endif

#ifdef CONFIG_SD_BOOT
	env_set("bootmode", "sdboot");
#endif

#ifdef CONFIG_QSPI_BOOT
	env_set("bootmode", "qspiboot");
#endif
	return 0;
}

unsigned int hb_gpio_get(void)
{
	return 0;
}

unsigned int hb_gpio_to_borad_id(unsigned int gpio_id)
{
	return 0;
}

int board_id_verify(unsigned int board_id)
{
	return 0;
}
