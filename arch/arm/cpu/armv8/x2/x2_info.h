#ifndef __X2_INFO_H__
#define __X2_INFO_H__

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
	unsigned int ddt1_csum;

	unsigned int ddt2_addr[4];		/* DDR 2D Training Firmware */
	unsigned int ddt2_csum;

	unsigned int ddrp_addr[4];		/* DDR Parameter for wakeup */
	unsigned int ddrp_size;
	unsigned int ddrp_csum;

	struct x2_image_hdr other_img[4];
	unsigned int other_laddr;

	unsigned int qspi_cfg;
	unsigned int emmc_cfg;

	unsigned int reserved[86];
};
#endif /* __X2_INFO_H__ */

