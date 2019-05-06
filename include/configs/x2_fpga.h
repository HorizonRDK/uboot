#ifndef __X2_FPGA_H__
#define __X2_FPGA_H__

#include <linux/sizes.h>

#if 0
#ifndef DEBUG
#define DEBUG
#endif /* DEBUG */
#endif /* #if 0 */

#ifndef COUNTER_FREQUENCY
#define COUNTER_FREQUENCY	20000000	/* System counter is 20MHz. */
#endif /* COUNTER_FREQUENCY */

#define CONFIG_ARMV8_SWITCH_TO_EL1
#define CONFIG_SYS_NONCACHED_MEMORY		(1 << 20)

/* sw_reg30 and sw_reg31 in pmu */
#define CPU_RELEASE_ADDR		(0xA6000278)

/* Support spl */
#ifdef CONFIG_SPL_BUILD

#define CONFIG_SPL_TEXT_BASE   0x80000000
#define CONFIG_SPL_MAX_SIZE    0x5000

#define CONFIG_SPL_BSS_START_ADDR  (CONFIG_SPL_TEXT_BASE + CONFIG_SPL_MAX_SIZE)
#define CONFIG_SPL_BSS_MAX_SIZE        0x800

#define CONFIG_SPL_STACK	(CONFIG_SPL_BSS_START_ADDR + 0x1000)

#define SPL_LOAD_OS_ADDR		0x80000
#define SPL_LOAD_DTB_ADDR		0x10000000

/* #define CONFIG_X2_AP_BOOT */
#define CONFIG_X2_YMODEM_BOOT

#else

#ifdef CONFIG_X2_BIFSD
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

#endif /*  CONFIG_X2_BIFSD */
#endif /* CONFIG_SPL_BUILD */

#define CONFIG_SYS_TEXT_BASE	0x20000000		/* Offset is 512MB*/

/* Physical Memory Map */
#define PHYS_SDRAM_1				0x00000000
#define PHYS_SDRAM_1_SIZE			0x80000000	/* 2G */

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE		PHYS_SDRAM_1_SIZE

#define CONFIG_SYS_INIT_SP_ADDR     (CONFIG_SYS_TEXT_BASE + SZ_8M)

/* Miscellaneous configurable options */
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x10000000)

#define CONFIG_ENV_SIZE    0x8000
#define CONFIG_BOOTCOMMAND	"run ${bootmode}"
#define CONFIG_SF_DUAL_FLASH
/*
#define CONFIG_ENV_IS_IN_FAT
#define FAT_ENV_INTERFACE               "mmc"
#define FAT_ENV_DEVICE_AND_PART         "1:1"
#define FAT_ENV_FILE                    "uboot.env"
#define CONFIG_FAT_WRITE
#define CONFIG_ENV_VARS_UBOOT_CONFIG
*/

/* Do not preserve environment */
#if !defined(CONFIG_ENV_IS_IN_FAT)
#define CONFIG_ENV_IS_NOWHERE		1
#endif

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/* Command line configuration */
#define CONFIG_SYS_MAXARGS	64	/* max command args */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_8M)

#define CONFIG_SYS_MAX_FLASH_BANKS	1

/* Serial setup */
#define UART_BAUDRATE_115200			115200

#define CONFIG_SYS_BAUDRATE_TABLE \
	{ 4800, 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_REMAKE_ELF
#define CONFIG_BOARD_LATE_INIT

/* boot mode select */
#define CONFIG_DDR_BOOT

#define CONFIG_PHY_MARVELL
#define CONFIG_BOUNCE_BUFFER
/*
 * This include file must after CONFIG_BOOTCOMMAND
 * and must include, otherwise will generate getenv failed
*/
#include <config_distro_bootcmd.h>

/* Initial environment variables */
#ifndef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"kernel_addr=0x80000\0" \
	"fdt_addr=0x4000000\0"  \
	"ddrboot=booti ${kernel_addr} - ${fdt_addr}\0"
#endif

/* #define X2_AUTOBOOT */

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

#endif /* __X2_FPGA_H__ */


