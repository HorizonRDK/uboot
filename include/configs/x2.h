#ifndef __X2_H__
#define __X2_H__

#include <linux/sizes.h>

#if 0
#ifndef DEBUG
#define DEBUG
#endif /* DEBUG */
#endif /* #if 0 */

#ifndef COUNTER_FREQUENCY
#define COUNTER_FREQUENCY	24000000	/* System counter is 20MHz. */
#endif /* COUNTER_FREQUENCY */

#define CONFIG_ARMV8_SWITCH_TO_EL1
#define CONFIG_SYS_NONCACHED_MEMORY		(1 << 20)

/* sw_reg30 and sw_reg31 in pmu */
#define CPU_RELEASE_ADDR		(0xA6000278)

/* Support spl */
#ifdef CONFIG_SPL_BUILD
#include "hb_config.h"

#define CONFIG_SPL_TEXT_BASE   0x80000000

#ifdef CONFIG_SPL_LOAD_FIT
#define CONFIG_SPL_MAX_SIZE    0x8C00

#define X2_SRAM_LOAD_ADDR	(0x8000B000)
#define X2_SRAM_LOAD_MAX	(0x4000)
#else
/* in fact, max size can be up to 0x7100, reserved 0x100 bytes for gap */
#define CONFIG_SPL_MAX_SIZE    0x7000

#define X2_SRAM_LOAD_ADDR	(0x80008000)
#define X2_SRAM_LOAD_MAX	(0x8000)
#endif /* CONFIG_SPL_LOAD_FIT */

#define CONFIG_SPL_BSS_START_ADDR  (CONFIG_SPL_TEXT_BASE + CONFIG_SPL_MAX_SIZE)
#define CONFIG_SPL_BSS_MAX_SIZE        0x600

#define CONFIG_SPL_STACK	(CONFIG_SPL_BSS_START_ADDR + 0xE00)

#define CONFIG_SPL_LOAD_FIT_ADDRESS	0x10000000

#ifdef CONFIG_MULTIMODE
#define SPL_LOAD_OS_ADDR		0x400000
#else
#define SPL_LOAD_OS_ADDR		0x80000
#endif
#define SPL_LOAD_DTB_ADDR		0x10000000

/* #define CONFIG_X2_LPDDR4_3200	(3200) */
/* #define CONFIG_X2_LPDDR4_2666	(2666) */
/* #define CONFIG_X2_LPDDR4_2400	(2400)*/
/* #define CONFIG_X2_LPDDR4_2133	(2133) */

/* The option for palladium */
/* #define CONFIG_SUPPORT_PALLADIUM */

/* The option for som board */
/* #define CONFIG_X2_SOM_BOARD */
/* #define CONFIG_X2_MONO_BOARD */
#ifdef HB_MMC_BOOT
#define CONFIG_X2_PM
#endif

#else

#ifdef CONFIG_HB_BIFSD
/* Generic Interrupt Controller Definitions */
#define CONFIG_GICV2
#define GICD_BASE	0x90001000
#define GICC_BASE	0x90002000

#define SP_BASE 0x80002000
#define GICD_SGI_PARAM 0x018000
#define GICD_TARGET_PARM 0x01010101
#define GICD_PRIORITY_PARAM 0x40404040
#define GICD_ACTIVE_PARAM 0x11111111

#define HR_TIMER
#define HR_TIMER_BASE 0xA1002000
#endif /*  CONFIG_HB_BIFSD */


#define CONFIG_PHY_MARVELL
#define CONFIG_BOUNCE_BUFFER

#endif /* CONFIG_SPL_BUILD */

#define CONFIG_SYS_SKIP_RELOC		/* skip relocation */
#define CONFIG_SYS_TEXT_BASE		0x02100000		/* Offset is 33MB*/
#define X2_USABLE_RAM_TOP		0x03100000		/* Top is 49MB*/

/* Physical Memory Map */
#define PHYS_SDRAM_1				0x00000000
#define PHYS_SDRAM_1_SIZE			0x40000000	/* 1G */

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE		PHYS_SDRAM_1_SIZE

/* For memtest */
#define CONFIG_SYS_MEMTEST_START    PHYS_SDRAM_1
#define CONFIG_SYS_MEMTEST_END      (CONFIG_SYS_MEMTEST_START + 0x60000000)

#define CONFIG_SYS_BOOTM_LEN		SZ_64M

/* Miscellaneous configurable options */
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x10000000)

#define CONFIG_ENV_SIZE    0x8000
/* #define CONFIG_BOOTCOMMAND	"run ${bootmode}" */
#define CONFIG_SF_DUAL_FLASH

#define X2_RESERVED_USER_SIZE	SZ_16K		/* reserved in top of TBL <= 24K */
#define HB_RESERVED_USER_ADDR	(X2_USABLE_RAM_TOP - X2_RESERVED_USER_SIZE)
/*
 * Do not preserve environment
 * #if !defined(CONFIG_ENV_IS_IN_FAT)
 * #define CONFIG_ENV_IS_NOWHERE		1
 * #endif
 */

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/* Command line configuration */
#define CONFIG_SYS_MAXARGS	64	/* max command args */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_8M)

#ifdef CONFIG_SYS_SKIP_RELOC
#define SZ_66K				0x00010800 /* TBL table + board info + global data */
#define CONFIG_SYS_INIT_SP_ADDR     (X2_USABLE_RAM_TOP - SZ_66K \
					- CONFIG_SYS_MALLOC_LEN - CONFIG_ENV_SIZE)
#else
#define CONFIG_SYS_INIT_SP_ADDR     (CONFIG_SYS_TEXT_BASE + SZ_1M)
#endif

#ifdef CONFIG_MULTIMODE
#define KERNEL_ADDR			0x400000
#else
#define KERNEL_ADDR			0x80000
#endif
#define FDT_ADDR			0x4000000
#define GZ_ADDR				0x8000000
#define LOAD_ADDR			0x6000000

#define CONFIG_SYS_MAX_FLASH_BANKS	1

/* Serial setup */
#define UART_BAUDRATE_115200			115200
#define UART_BAUDRATE_921600			921600

#define CONFIG_SYS_BAUDRATE_TABLE \
	{ 4800, 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_REMAKE_ELF
#define CONFIG_BOARD_LATE_INIT

/* boot mode select */
#define CONFIG_DDR_BOOT

/*
 * This include file must after CONFIG_BOOTCOMMAND
 * and must include, otherwise will generate getenv failed
*/
#include <config_distro_bootcmd.h>

#define CONFIG_BOOTARGS "earlycon console=ttyS0 clk_ignore_unused "
#define CONFIG_BOOTCOMMAND "run mmcload;send_id;run unzipimage;" \
		"ion_modify ${ion_size};mem_modify ${mem_size};run ddrboot;"

/* Initial environment variables */
#ifndef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
    "kernel_addr=" __stringify(KERNEL_ADDR) "\0" \
    "fdt_addr=" __stringify(FDT_ADDR) "\0" \
    "gz_addr=" __stringify(GZ_ADDR) "\0" \
    "bootfile=Image.gz\0" \
    "fdtimage=hobot-x2-soc.dtb\0" \
    "bootargs=" CONFIG_BOOTARGS \
        "root=/dev/mmcblk0p6 rootfstype=ext4 rw rootwait\0" \
    "mmcload=mmc rescan;" \
        "ext4load mmc 0:4 ${gz_addr} ${bootfile};" \
        "ext4load mmc 0:4 ${fdt_addr} ${fdtimage}\0" \
    "unzipimage=unzip ${gz_addr} ${kernel_addr}\0" \
    "ddrboot=booti ${kernel_addr} - ${fdt_addr}\0" \
    "load_addr=" __stringify(LOAD_ADDR) "\0"
#endif

/* #define HB_AUTOBOOT */

#define BIF_SHARE_REG_OFF       0xA1006010
#define BOOT_STAGE0_VAL         0x5a
#define BOOT_STAGE1_VAL         0x6a
#define BOOT_STAGE2_VAL         0x7a
#define BOOT_STAGE3_VAL         0x8a
#define BOOT_WAIT_VAL           0xaa

#define BIF_SHARE_REG_BASE      0xA1006000
#define BIF_SHARE_REG(x)        (BIF_SHARE_REG_BASE + ((x) << 2))
#define BOOT_VIA_NFS            (0x55)

#define NETCONSOLE_CONFIG_VALID	(0x55)

#define CONFIG_UDP_CHECKSUM

#endif /* __X2_H__ */
