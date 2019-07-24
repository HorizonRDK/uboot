#include <common.h>
#include <sata.h>
#include <ahci.h>
#include <scsi.h>
#include <malloc.h>
#include <asm/io.h>

#include <asm/armv8/mmu.h>
#include <asm/arch/x2_reg.h>

#include <asm/arch/ddr.h>
#include <configs/x2.h>
#include "../arch/arm/cpu/armv8/x2/x2_info.h"

unsigned int sys_sdram_size = 0x80000000; /* 2G */
unsigned int x2_src_boot = 1;

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region x2_mem_map[] = {
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
	},{
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = x2_mem_map;
static void x2_boot_src_init(void)
{
	unsigned int reg;

	reg = reg32_read(X2_GPIO_BASE + X2_STRAP_PIN_REG);
	x2_src_boot = PIN_2NDBOOT_SEL(reg);

	printf("x2_gpio_boot_mode is %02x \n", x2_src_boot);
}

static void system_sdram_size_init(void)
{
	unsigned int board_id = 0;
	unsigned int gpio_id = 0;
	struct x2_info_hdr* boot_info = (struct x2_info_hdr*) 0x10000000;

	board_id = boot_info->board_id;

	if (board_id == X2_GPIO_MODE) {
		gpio_id = x2_gpio_get();

		board_id = x2_gpio_to_borad_id(gpio_id);

		if (board_id == 0xff) {
			printf("error: gpio id %02x not support \n", gpio_id);
			return;
		}
	} else {
		if (board_id_verify(board_id) != 0) {
			printf("error: board id %02x not support \n", board_id);
			return;
		}
	}

	switch (board_id) {
	case X2_SVB_BOARD_ID:
	case J2_SVB_BOARD_ID:
	case J2_SOM_BOARD_ID:
	case X2_MONO_BOARD_ID:
	case J2_SOM_DEV_ID:
	case J2_SOM_SK_ID:
	case QUAD_BOARD_ID:
		sys_sdram_size = 0x80000000; /* 2G */
		break;
	case J2_MM_BOARD_ID:
	case X2_96BOARD_ID:
		sys_sdram_size = 0x40000000; /* 1G */
		break;
	default:
		sys_sdram_size = 0x40000000; /* 1G */
		break;
	}
}

static void x2_mem_map_init(void)
{
	system_sdram_size_init();

	x2_mem_map[0].size = sys_sdram_size;
	printf("sys_sdram_size = %02x \n", sys_sdram_size);
}

int dram_init(void)
{
	x2_boot_src_init();

	if ((x2_src_boot == PIN_2ND_EMMC) || (x2_src_boot == PIN_2ND_SF)) {
		x2_mem_map_init();

		gd->ram_size = sys_sdram_size;
	} else {
		gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	}

	return 0;
}

int board_init(void)
{
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
