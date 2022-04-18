/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#ifndef __X3_H__
#define __X3_H__

#include <linux/sizes.h>

#if 0
#ifndef DEBUG
#define DEBUG
#endif /* DEBUG */
#endif /* #if 0 */

#define CONFIG_BOUNCE_BUFFER

#ifndef COUNTER_FREQUENCY
#define COUNTER_FREQUENCY	24000000	/* System counter is 24MHz. */
#endif /* COUNTER_FREQUENCY */

#define CONFIG_ARMV8_SWITCH_TO_EL1
#define CONFIG_SYS_NONCACHED_MEMORY		(1 << 20)

#ifdef CONFIG_PARALLEL_CPU_CORE_ONE
#define CPU_NUM                             (4)
#define COR0_PWR_CTRL_REG                   (0xA1000490UL)
#define COR1_PWR_CTRL_REG                   (0xA1000494UL)
#define COR2_PWR_CTRL_REG                   (0xA1000498UL)
#define COR3_PWR_CTRL_REG                   (0xA100049CUL)
#define CORn_PWR_CTRL_REG_PWR_ON            (0x0001)
#define CORn_PWR_CTRL_REG_STS               (0x0F00)
#define CORn_PWR_CTRL_REG_STS_COMPLETE      (0x0800)

#define PMU_CPU_WFI_STS                     (0xA6000048)
#define X2A_CPU1_POWEROFF                   (0xA6000240)
#define X2A_CPU2_POWEROFF                   (0xA6000244)
#define X2A_CPU3_POWEROFF                   (0xA6000248)
#define CPU_RELEASE_ADDR		            X2A_CPU1_POWEROFF
#else
/* sw_reg30 and sw_reg31 in pmu */
#define CPU_RELEASE_ADDR		(0xA6000278)
#endif /*CONFIG_PARALLEL_CPU_CORE_ONE*/

#define CONFIG_SYS_SKIP_RELOC		/* skip relocation */
#define X3_USABLE_RAM_TOP		0x7000000	/* Top is + 112MB */

/* Physical Memory Map */
#define PHYS_SDRAM_1				0x200000
#define PHYS_SDRAM_1_SIZE			(0x80000000 - PHYS_SDRAM_1)	/* 2G - 2M*/
#define PHYS_SDRAM_2				0x100000000
#define PHYS_SDRAM_2_SIZE			0x80000000	/* 2G */

#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED		PHYS_SDRAM_1_SIZE /* 2G - 2M*/

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

#define X3_RESERVED_USER_SIZE	SZ_16K		/* reserved in top of TBL <= 24K */
#define HB_RESERVED_USER_ADDR	(X3_USABLE_RAM_TOP - X3_RESERVED_USER_SIZE)
/*
 * Do not preserve environment
 * #if !defined(CONFIG_ENV_IS_IN_FAT)
 * #define CONFIG_ENV_IS_NOWHERE		1
 * #endif
 */

#ifdef CONFIG_ENV_IS_IN_UBI
#define CONFIG_ENV_UBI_PART "env"
#define CONFIG_ENV_UBI_VOLUME "ubootenv"
#define CONFIG_ENV_UBI_VOLUME_REDUND "ubootenvbak"
#endif

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
/* TODO: The offset of uboot environment is located at the end 64KiB of */
/* UBoot partition. Please Adjust as Needed */
#define CONFIG_ENV_OFFSET 0x280000
#ifdef CONFIG_ENV_SIZE
#undef CONFIG_ENV_SIZE
#endif
#define CONFIG_ENV_SIZE   0x10000
#define CONFIG_ENV_SECT_SIZE 256
#endif /* CONFIG_ENV_IS_IN_SPI_FLASH */

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	1024	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/* Command line configuration */
#define CONFIG_SYS_MAXARGS	64	/* max command args */
#define CONFIG_NET_RETRY_COUNT	100	/* for rndis arp retry count exceeded */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_32M)

#ifdef CONFIG_SYS_SKIP_RELOC
#define SZ_66K				0x00010800 /* TBL table + board info + global data */
#define CONFIG_SYS_INIT_SP_ADDR     (X3_USABLE_RAM_TOP - SZ_66K \
					- CONFIG_SYS_MALLOC_LEN - CONFIG_ENV_SIZE)
#else
#define CONFIG_SYS_INIT_SP_ADDR     (CONFIG_SYS_TEXT_BASE + SZ_1M)
#endif

#ifdef CONFIG_MULTIMODE
#define KERNEL_ADDR			0x400000
#else
#define KERNEL_ADDR			0x200000
#endif
#define FDT_ADDR			0x3C00000
#define GZ_ADDR				0x8000000
#define LOAD_ADDR			X3_USABLE_RAM_TOP

#define CONFIG_SYS_MAX_FLASH_BANKS	1

/* DFU class support */
#define CONFIG_SYS_DFU_DATA_BUF_SIZE	(SZ_4M)

/* Serial setup */
#define UART_BAUDRATE_115200			115200
#define UART_BAUDRATE_921600			921600

#define CONFIG_SYS_BAUDRATE_TABLE \
    { 4800, 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_REMAKE_ELF
#define CONFIG_BOARD_LATE_INIT

/* boot mode select */
#define CONFIG_DDR_BOOT
#define BOOTIMG_ADDR 0x10000000
#define CONFIG_BOOTARGS "console=ttyS0"
/*
 * #define CONFIG_BOOTCOMMAND "run mmcload;send_id;run unzipimage;" \
 *		"ion_modify ${ion_size};mem_modify ${mem_size};run ddrboot;"
 */
#if defined(CONFIG_HB_WATCHDOG) && \
    !(defined(CONFIG_HB_AP_BOOT) || defined(CONFIG_HB_YMODEM_BOOT))
#define HB_SET_WDT     "watchdog on;"
#else
#define HB_SET_WDT     ""
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOTSCR_ADDR			0x3C10000
#define RAMDISK_ADDR			0x4000000

#define ENV_MEM_LAYOUT_SETTINGS \
	"kernel_addr_r="__stringify(KERNEL_ADDR)"\0" \
	"scriptaddr="__stringify(BOOTSCR_ADDR)"\0" \
	"fdt_addr_r="__stringify(FDT_ADDR)"\0" \
	"ramdisk_addr_r="__stringify(RAMDISK_ADDR)"\0"

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 2)

#define CONFIG_BOOTCOMMAND HB_SET_WDT "run distro_bootcmd"
#else
#define CONFIG_BOOTCOMMAND HB_SET_WDT "part size mmc 0 %s bootimagesize;"\
	"part start mmc 0 %s bootimageblk;mmc read "__stringify(BOOTIMG_ADDR) \
	" ${bootimageblk} ${bootimagesize};bootm "__stringify(BOOTIMG_ADDR)";"
#endif

#if (CONFIG_BOOTDELAY == 0) && defined(CONFIG_PARALLEL_CPU_CORE_ONE)
/*Only run bootm command*/
#define BOOTCOMMAND_DIRECT_BOOTM "bootm "__stringify(BOOTIMG_ADDR)";"
#endif /*CONFIG_PARALLEL_CPU_CORE_ONE*/
/*
 * This include file must after CONFIG_BOOTCOMMAND
 * and must include, otherwise will generate getenv failed
*/
#include <config_distro_bootcmd.h>
/*
 * #define CONFIG_BOOTARGS "earlycon loglevel=8 console=ttyS0 "
 	" clk_ignore_unused "
 * #define CONFIG_BOOTCOMMAND "run mmcload;send_id;run unzipimage;run ddrboot;"
 * #define CONFIG_BOOTCOMMAND "run mmcload;send_id;run unzipimage;" \
 *		"ion_modify ${ion_size};mem_modify ${mem_size};run ddrboot;"
 */

/* USB Device Firmware Upgrade support */
#define DFU_ALT_INFO_EMMC \
	"disk.img raw 0 30597120;" \
	"veeprom part 0 1;" \
	"sbl part 0 2;" \
	"ddr part 0 3;" \
	"bl31 part 0 4;" \
	"uboot part 0 5;" \
	"vbmeta part 0 6;" \
	"boot part 0 7;" \
	"recovery part 0 8;" \
	"system part 0 9;" \
	"bpu part 0 10;" \
	"app part 0 11;" \
	"userdata part 0 12\0"

#define DFU_ALT_INFO_SPINAND \
	"disk.img raw 0 8000000;" \
	"bootloader part 1;" \
	"boot part 2;" \
	"system part 3;" \
	"userdata part 4\0"

/* default partition table */
#ifndef PARTS_DEFAULT
/* Define the default GPT table for eMMC */
#define PARTS_DEFAULT \
	/* default partitions */ \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=veeprom,start=17K,size=2K,uuid=${uuid_gpt_veeprom};" \
	"name=sbl,size=256K,uuid=${uuid_gpt_sbl};" \
	"name=ddr,size=128K,uuid=${uuid_gpt_ddr};" \
	"name=bl31,size=512K,uuid=${uuid_gpt_bl31};" \
	"name=uboot,size=2M,uuid=${uuid_gpt_uboot};" \
	"name=bpu,size=128K,uuid=${uuid_gpt_bpu};" \
	"name=vbmeta,size=128K,uuid=${uuid_gpt_vbmeta};" \
	"name=boot,size=25M,uuid=${uuid_gpt_boot};" \
	"name=recovery,size=128K,uuid=${uuid_gpt_recovery};" \
	"name=system,size=200M,uuid=${uuid_gpt_system};" \
	"name=app,size=256M,uuid=${uuid_gpt_app};" \
	"name=userdata,size=-,uuid=${uuid_gpt_userdata}"
#endif /* PARTS_DEFAULT */

/* Initial environment variables */
#ifndef CONFIG_EXTRA_ENV_SETTINGS
#ifdef CONFIG_DISTRO_DEFAULTS
#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_MEM_LAYOUT_SETTINGS \
	BOOTENV
#else
#define CONFIG_EXTRA_ENV_SETTINGS \
	"kernel_addr=" __stringify(KERNEL_ADDR) "\0" \
	"fdt_addr=" __stringify(FDT_ADDR) "\0" \
	"gz_addr=" __stringify(GZ_ADDR) "\0" \
	"bootimage=Image\0" \
	"bootfile=Image.gz\0" \
	"fdtimage=hobot-xj3-soc.dtb\0" \
	"bootargs=" CONFIG_BOOTARGS "\0" \
	"mmcload=mmc rescan;" \
	    "ext4load mmc 0:4 ${gz_addr} ${bootfile};" \
	    "ext4load mmc 0:4 ${fdt_addr} ${fdtimage}\0" \
	"unzipimage=unzip ${gz_addr} ${kernel_addr}\0" \
	"ddrboot=booti ${kernel_addr} - ${fdt_addr}\0" \
	"load_addr=" __stringify(LOAD_ADDR) "\0" \
	"usbboot2=usb start;" \
	    "fatload usb 0:0 ${kernel_addr} ${bootimage};" \
	    "fatload usb 0:0 ${fdt_addr} ${fdtimage};" \
	    "run ddrboot\0" \
	"cdc_connect_timeout=360\0" \
	"partitions=" PARTS_DEFAULT "\0"
#endif
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

#define CONFIG_PREBOOT

#define DTS_POWER_MANAGEMENT_PATH   \
    "/soc/syscon@0xA6000000/power-management"
#define EFUSE_VERIFY_FALID_FLAG 0x4645 //"EF" efuse verify failed
#endif /* __X3_H__ */
