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

#define PIN_1STBOOT_SEL(x)		((x) & 0x1)

#define PIN_BOARD_SEL(x,y)		((((x) >> 8) & 0x3) << 4)\
		| ((((y) >> 7) & 0x1) << 3) | ((((y) >> 10) & 0x1) << 2)\
		| ((((y) >> 11) & 0x1) << 1) | (((y) >> 12) & 0x1)

/* Boot strap Bit0 is reserved */
#define PIN_2NDBOOT_SEL(x)		(((x) >> 1) & 0x7)

#define PIN_2ND_EMMC		0x0
#define PIN_2ND_NAND		0x1
#define PIN_2ND_AP		    0x2
#define PIN_2ND_UART		0x3
#define PIN_2ND_USB		    0x4
#define PIN_2ND_NOR		    0x5
#define PIN_2ND_USB_BURN    0x6

//#define HB_RESERVED_USER_SIZE	SZ_16K		/* reserved in top of TBL <= 24K */
//#define HB_RESERVED_USER_ADDR	(HB_USABLE_RAM_TOP - HB_RESERVED_USER_SIZE)

#define HB_BOOTINFO_ADDR	(HB_RESERVED_USER_ADDR)
#define HB_DTB_CONFIG_ADDR	(HB_BOOTINFO_ADDR + 0x1000)
#define DTB_MAPPING_ADDR	0x140000
#define DTB_MAPPING_SIZE	0x400
#define DTB_MAX_NUM		64
#define KERNEL_HEAD_ADDR	0x140000
#define RECOVERY_HEAD_ADDR	0x6D0000

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

/* x3 and j3 board type */
#define X3_DVB_HYNIX_ID		0x1001
#define X3_DVB_MICROM_ID	0x1002
#define X3_CVB_MICROM_ID	0x2001
#define J3_DVB_HYNIX_ID		0x3001
#define J3_DVB_MICROM_ID	0x3002

#define PIN_DEV_MODE_SEL(x)		(((x) >> 4) & 0x1)

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
	unsigned char dtb_name[32];
};

struct hb_kernel_hdr {
	unsigned int Image_addr;		/* Address in storage */
	unsigned int Image_size;
	unsigned int Recovery_addr;		/* Address in storage */
	unsigned int Recovery_size;
	unsigned int dtb_number;

	struct hb_dtb_hdr dtb[DTB_MAX_NUM];

	unsigned int reserved[11];
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

#endif /* __HB_INFO_H__ */
