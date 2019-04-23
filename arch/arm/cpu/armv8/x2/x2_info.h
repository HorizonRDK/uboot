#ifndef __X2_INFO_H__
#define __X2_INFO_H__

#define GPIO_GRP0_REG		0x000
#define GPIO_GRP1_REG		0x010
#define GPIO_GRP2_REG		0x020
#define GPIO_GRP3_REG		0x030
#define GPIO_GRP4_REG		0x040
#define GPIO_GRP5_REG		0x050
#define GPIO_GRP6_REG		0X060
#define GPIO_GRP7_REG		0x070

#define STRAP_PIN_REG		0x140

#define X2_SYSCNT_BASE		(0xA6001000)
#define X2_GPIO_BASE		(0xA6003000)

#define PIN_1STBOOT_SEL(x)		((x) & 0x1)

#define PIN_2NDBOOT_SEL(x)		(((x) >> 1) & 0x3)

#define PIN_BOARD_SEL(x,y)		((((x) >> 8) & 0x3) << 4)\
		| ((((y) >> 7) & 0x1) << 3) | ((((y) >> 10) & 0x1) << 2)\
		| ((((y) >> 11) & 0x1) << 1) | (((y) >> 12) & 0x1)


#define PIN_2NDBOOT_SEL(x)		(((x) >> 1) & 0x3)
#define PIN_2ND_EMMC		0x0
#define PIN_2ND_SF			0x1
#define PIN_2ND_AP			0x2
#define PIN_2ND_UART		0x3

#define X2_BOOTINFO_ADDR	(0x10000000)
#define X2_DTB_CONFIG_ADDR	(0x10001000)

#define X2_BOARD_SVB 0
#define X2_BOARD_SOM 1

#define x2_BOOTINFO_MODE	1
#define X2_GPIO_MODE		0

#define X2_MONO_BOARD_ID	0x202
#define X2_MONO_GPIO_ID		0x3C

#define J2_SOM_GPIO_ID		0x30
#define J2_SOM_BOARD_ID		0x201
#define X2_SVB_BOARD_ID		0x100
#define J2_SVB_BOARD_ID		0x200
#define X2_SOM_3V3_ID		0x103

#define PIN_DEV_MODE_SEL(x)		(((x) >> 4) & 0x1)

struct x2_image_hdr {
	unsigned int img_addr;
	unsigned int img_size;
	unsigned int img_csum;
};

struct x2_info_hdr {
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

	struct x2_image_hdr other_img[4];
	unsigned int other_laddr;

	unsigned int qspi_cfg;
	unsigned int emmc_cfg;
	unsigned int board_id;

	unsigned int reserved[79];
};

struct x2_dtb_hdr {
	unsigned int board_id;
	unsigned int gpio_id;
	unsigned int dtb_addr;		/* Address in storage */
	unsigned int dtb_size;
	unsigned char dtb_name[32];
};

struct x2_kernel_hdr {
	unsigned int Image_addr;		/* Address in storage */
	unsigned int Image_size;
	unsigned int dtb_number;

	struct x2_dtb_hdr dtb[16];

	unsigned int reserved[61];
};

#endif /* __X2_INFO_H__ */
