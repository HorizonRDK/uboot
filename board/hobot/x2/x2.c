/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#include <common.h>
#include <sata.h>
#include <ahci.h>
#include <scsi.h>
#include <malloc.h>
#include <asm/io.h>

#include <asm/armv8/mmu.h>
#include <asm/arch/hb_reg.h>
#include <asm/arch/ddr.h>
#include <configs/x2.h>
#include <hb_info.h>
#ifndef CONFIG_SPL_BUILD
extern struct spi_flash *flash;
unsigned int sys_sdram_size = 0x80000000; /* 2G */
unsigned int hb_src_boot = 1;
bool recovery_sys_enable = true;

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
	hb_src_boot = PIN_2NDBOOT_SEL(reg);

	// printf("x2_gpio_boot_mode is %02x \n", hb_src_boot);
}

static unsigned int x2_board_id[] = {
        0x100, 0x200, 0x201, 0x102, 0x103, 0x101, 0x202, 0x203, 0x204, 0x205,
        0x300, 0x301, 0x302, 0x303, 0x304, 0x400, 0x401, 0x104, 0x105, 0x106
};

static unsigned int x2_gpio_id[] = {
        0xff, 0xff, 0x30, 0x20, 0x10, 0x00, 0x3C, 0xff, 0xff, 0xff,
        0xff, 0x34, 0x36, 0x35, 0x37, 0xff, 0xff, 0xff, 0xff, 0xff
};

int board_id_verify(unsigned int board_id)
{
        int i = 0;

        for (i = 0; i < ARRAY_SIZE(x2_board_id); i++) {
                if (board_id == x2_board_id[i])
                        return 0;
        }

        return -1;
}

unsigned int hb_gpio_to_board_id(unsigned int gpio_id)
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


unsigned int hb_gpio_get(void)
{
        unsigned int reg_x, reg_y;

        reg_x = reg32_read(X2_GPIO_BASE + GPIO_GRP5_REG);
        reg_y = reg32_read(X2_GPIO_BASE + GPIO_GRP4_REG);

        return PIN_BOARD_SEL(reg_x, reg_y);
}


static void system_sdram_size_init(void)
{
	unsigned int board_id = 0;
	unsigned int gpio_id = 0;
	struct hb_info_hdr* boot_info = (struct hb_info_hdr*) HB_BOOTINFO_ADDR;

	board_id = boot_info->board_id;

	if (board_id == HB_GPIO_MODE) {
		gpio_id = hb_gpio_get();

		board_id = hb_gpio_to_board_id(gpio_id);

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
	case QUAD_BOARD_ID:
		sys_sdram_size = 0x80000000; /* 2G */
		break;
	case J2_MM_BOARD_ID:
        case J2_MM_S202_BOARD_ID:
	case X2_96BOARD_ID:
	case J2_SOM_SK_ID:
	case J2_SOM_SAM_ID:
		sys_sdram_size = 0x40000000; /* 1G */
		break;
	case X2_DEV_512M_BOARD_ID:
		sys_sdram_size = 0x20000000; /* 512M */
		break;
	default:
		sys_sdram_size = 0x40000000; /* 1G */
		break;
	}

	if ((board_id == X2_MONO_BOARD_ID) && (hb_src_boot == PIN_2ND_SF))
		sys_sdram_size = 0x20000000; /* 512M */

	//START4[prj_j2quad]
	//QUAD alike board force setting src as emmc even for nor+eMMC case
	if (board_id==QUAD_BOARD_ID)
		hb_src_boot = PIN_2ND_EMMC;
	//END4[prj_j2quad]
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

	if ((hb_src_boot == PIN_2ND_EMMC) || (hb_src_boot == PIN_2ND_SF)) {
		x2_mem_map_init();

		gd->ram_size = sys_sdram_size;
	} else {
		gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	}

	return 0;
}
#endif
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

#ifdef X2_USABLE_RAM_TOP
ulong board_get_usable_ram_top(ulong total_size)
{
	return X2_USABLE_RAM_TOP;
}
#endif
