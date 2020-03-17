/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#ifndef __HB_INFO_H__
#define __HB_INFO_H__

#define HB_SWINFO_MEM_ADDR                      0x020ff000
#define HB_SWINFO_MEM_MAGIC                     0x57534248
#define HB_SWINFO_BOOT_OFFSET           0x4
#define HB_SWINFO_DUMP_OFFSET           0x8
#define HB_SWINFO_BOOT_SPLONCE          0x1
#define HB_SWINFO_BOOT_UBOOTONCE        0x2
#define HB_SWINFO_BOOT_SPLWAIT          0x3
#define HB_SWINFO_BOOT_UBOOTWAIT        0x4
#define HB_SWINFO_BOOT_UDUMPTF          0x5
#define HB_SWINFO_BOOT_UDUMPEMMC        0x6

/* auto detection */
#define AUTO_DETECTION	0x0

/* ddr manufacture */
#define DDR_MANU_HYNIX		0x1
#define DDR_MANU_MICRON		0x2
#define PIN_DDR_TYPE_SEL(x)	((x) & 0x1)

/* ddr type */
#define DDR_TYPE_LPDDR4		0x1
#define DDR_TYPE_LPDDR4X	0x2
#define DDR_TYPE_DDR4		0x3
#define DDR_TYPE_DDR3L		0x4

/* ddr frequency */
#define DDR_FREQC_667	0x1
#define DDR_FREQC_1600	0x2
#define DDR_FREQC_2133	0x3
#define DDR_FREQC_2666	0x4
#define DDR_FREQC_3200	0x5
#define DDR_FREQC_3733	0x6
#define DDR_FREQC_4266	0x7

/* ddr capacity */
#define DDR_CAPACITY_1G		0x1
#define DDR_CAPACITY_2G		0x2
#define DDR_CAPACITY_4G		0x3

/* som type */
#define SOM_TYPE_X3		0x1
#define SOM_TYPE_J3		0x2

/* base board type */
#define BASE_BOARD_X3_DVB		0x1
#define BASE_BOARD_J3_DVB		0x2
#define BASE_BOARD_CVB			0x3
#define BASE_BOARD_CUSTOMER_BOARD	0x4

/* Boot strap Bit0 is reserved */
#define PIN_1STBOOT_SEL(x)		((x) & 0x1)
#define PIN_2NDBOOT_SEL(x)		(((x) >> 1) & 0x7)
/* Reuse strap Bit15 as fastboot mode except usb boot */
#define PIN_FASTBOOT_SEL(x)		(((x) >> 16) & 0x1)
#define DDR_MANUF_SEL(x)  (((x) >> 28) & 0xf)
#define DDR_TYPE_SEL(x) (((x) >> 24) & 0xf)
#define DDR_FREQ_SEL(x)  (((x) >> 20) & 0xf)
#define DDR_CAPACITY_SEL(x)  (((x) >> 16) & 0xf)
#define SOM_TYPE_SEL(x)  (((x) >> 8) & 0xff)
#define BASE_BOARD_SEL(x)  ((x) & 0xff)
#define PIN_BASE_BOARD_SEL(x)	((((x >> 14) & 0x1) << 0x1) | \
	((x >> 12) & 0x1))
#define PIN_BOARD_SEL(x, y)		((((x) >> 8) & 0x3) << 4)\
		| ((((y) >> 7) & 0x1) << 3) | ((((y) >> 10) & 0x1) << 2)\
		| ((((y) >> 11) & 0x1) << 1) | (((y) >> 12) & 0x1)

#define PIN_2ND_EMMC		0x0
#define PIN_2ND_NAND		0x1
#define PIN_2ND_AP		    0x2
#define PIN_2ND_UART		0x3
#define PIN_2ND_USB		    0x4
#define PIN_2ND_NOR		    0x5
#define PIN_2ND_USB_BURN    0x6

//#define HB_RESERVED_USER_SIZE	SZ_16K		/* reserved in top of TBL <= 24K */
//#define HB_RESERVED_USER_ADDR	(HB_USABLE_RAM_TOP - HB_RESERVED_USER_SIZE)

#define HB_BOOTINFO_ADDR	(0x10000000)
#define HB_DTB_CONFIG_ADDR	(HB_BOOTINFO_ADDR + 0x1000)
#define DTB_MAPPING_ADDR	0x140000
#define DTB_MAX_NUM		20
#define DTB_MAPPING_SIZE	0x400
#define DTB_NAME_MAX_LEN	32
#define DTB_RESERVE_SIZE	((DTB_MAPPING_SIZE - (DTB_MAX_NUM*48) - 20)/4)
#define KERNEL_HEAD_ADDR	0x234400
#define RECOVERY_HEAD_ADDR	0x1634400

#define X2_BOARD_SVB 0
#define X2_BOARD_SOM 1

#define X2_MONO_BOARD_ID	0x202
#define X2_MONO_GPIO_ID		0x3C

#define J2_SOM_GPIO_ID		0x30
#define J2_SOM_BOARD_ID		0x201
#define J2_SOM_DEV_ID		0x203
#define J2_SOM_SK_ID		0x204
#define J2_SOM_SAM_ID		0x205
#define X2_SVB_BOARD_ID		0x100
#define J2_SVB_BOARD_ID		0x200
#define X2_DEV_BOARD_ID		0x101
#define X2_SOM_3V3_ID		0x103
#define X2_SOM_ID		0x104
#define X2_96BOARD_ID		0x105
#define X2_DEV_512M_BOARD_ID	0x106
#define QUAD_BOARD_ID		0x300
#define J2_MM_BOARD_ID		0x400
#define J2_MM_S202_BOARD_ID     0x401

#define PIN_DEV_MODE_SEL(x)		(((x) >> 4) & 0x1)

#ifdef CONFIG_ENV_IS_IN_UBI
#define CONFIG_ENV_UBI_PART "sys"
#define CONFIG_ENV_UBI_VOLUME "ubootenv"
#define CONFIG_ENV_UBI_VOLUME_REDUND "ubootenvbak"
#endif

extern uint32_t x3_board_id;
extern uint32_t x3_som_type;

struct hb_image_hdr {
	unsigned int img_addr;
	unsigned int img_size;
	unsigned int img_csum;
};

struct hb_info_hdr {
	unsigned int manu_id;		/* Manualfacture identify */
	unsigned int chip_id;		/* Chip identify */
	unsigned int fw_ver;		/* Firmware version */
	unsigned int info_csum;	/* A check sum of the structure */

	unsigned int boot_addr[4];		/* Address in storage */
	unsigned int boot_size;
	unsigned int boot_csum;
	unsigned int boot_laddr;		/* Load address */

	unsigned int ddt1_addr[4];		/* DDR 1D Training Firmware */
	unsigned int ddt1_imem_size;
	unsigned int ddt1_dmem_size;
	unsigned int ddt1_imem_csum;
	unsigned int ddt1_dmem_csum;

	unsigned int ddt2_addr[4];		/* DDR 2D Training Firmware */
	unsigned int ddt2_imem_size;
	unsigned int ddt2_dmem_size;
	unsigned int ddt2_imem_csum;
	unsigned int ddt2_dmem_csum;

	unsigned int ddrp_addr[4];		/* DDR Parameter for wakeup */
	unsigned int ddrp_size;
	unsigned int ddrp_csum;

	struct hb_image_hdr other_img[4];
	unsigned int other_laddr;

	unsigned int qspi_cfg;
	unsigned int emmc_cfg;
	unsigned int board_id;

	unsigned int reserved[79];
};

struct hb_dtb_hdr {
	unsigned int board_id;
	unsigned int gpio_id;
	unsigned int dtb_addr;		/* Address in storage */
	unsigned int dtb_size;
	unsigned char dtb_name[DTB_NAME_MAX_LEN];
};

struct hb_kernel_hdr {
	unsigned int Image_addr;		/* Address in storage */
	unsigned int Image_size;
	unsigned int Recovery_addr;		/* Address in storage */
	unsigned int Recovery_size;
	unsigned int dtb_number;

	struct hb_dtb_hdr dtb[DTB_MAX_NUM];

	unsigned int reserved[DTB_RESERVE_SIZE];
};

struct hb_flash_kernel_hdr {
	unsigned char magic[16];			/* FLASH0 */
	unsigned int Image_addr;
	unsigned int Image_size;
	unsigned char dtbname[32];
	unsigned int dtb_addr;
	unsigned int dtb_size;
};

int hb_boot_mode_get(void);
int hb_fastboot_key_pressed(void);
uint32_t hb_board_type_get(void);
uint32_t hb_base_board_type_get(void);

#endif /* __HB_INFO_H__ */
