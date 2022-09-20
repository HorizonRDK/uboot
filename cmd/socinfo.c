/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
*/
#include <linux/ctype.h>
#include <linux/types.h>
#include <common.h>
#include <mapmem.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <fdt_support.h>
#include <x2_fuse_regs.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-x2/ddr.h>
#include <socinfo.h>
#include <hb_info.h>
#include <scomp.h>

#define SCRATCHPAD	1024
#ifdef CONFIG_MMC_TUNING_DATA_TRANS
DECLARE_GLOBAL_DATA_PTR;
#endif
static uint32_t x3_ddr_vender;
static uint32_t x3_ddr_size;
static uint32_t x3_ddr_type;
static uint32_t x3_ddr_freq;
extern uint32_t x3_ddr_part_num;
static uint32_t x3_origin_board_id;
static uint32_t hb_base_board_type;

static int find_fdt(unsigned long fdt_addr);
static int valid_fdt(struct fdt_header **blobp);

extern struct hb_uid_hdr hb_unique_id;

void set_hb_fdt_addr(ulong addr)
{
	void *buf;

	buf = map_sysmem(addr, 0);
	hb_dtb = buf;
	env_set_hex("fdtaddr", addr);
}

static int valid_fdt(struct fdt_header **blobp)
{
	const void *blob = *blobp;
	int err;

	if (blob == NULL) {
		printf ("The address of the fdt is invalid (NULL).\n");
		return 0;
	}

	err = fdt_check_header(blob);
	if (err == 0)
		return 1;	/* valid */

	if (err < 0) {
		printf("libfdt fdt_check_header(): %s", fdt_strerror(err));
		/*
		 * Be more informative on bad version.
		 */
		if (err == -FDT_ERR_BADVERSION) {
			if (fdt_version(blob) <
					FDT_FIRST_SUPPORTED_VERSION) {
				printf (" - too old, fdt %d < %d",
						fdt_version(blob),
						FDT_FIRST_SUPPORTED_VERSION);
			}
			if (fdt_last_comp_version(blob) >
					FDT_LAST_SUPPORTED_VERSION) {
				printf (" - too new, fdt %d > %d",
						fdt_version(blob),
						FDT_LAST_SUPPORTED_VERSION);
			}
		}
		printf("\n");
		*blobp = NULL;
		return 0;
	}
	return 1;
}

#if 0
static int socuid_read(u32 word, u32 *val)
{
	unsigned int i = 0, rv;
	if (word > (X2_EFUSE_WRAP_DATA_LEN-1)) {
		printf("overflow, total number is %d, word can be 0-%d\n",
				X2_EFUSE_WRAP_DATA_LEN, X2_EFUSE_WRAP_DATA_LEN-1);
		return -EINVAL;
	}
	rv = readl((void *)X2_SWRST_REG_BASE);
	rv &= (~X2_EFUSE_OP_REPAIR_RST);
	writel(rv, (void *)X2_SWRST_REG_BASE);
	x2efuse_wr(X2_EFUSE_WRAP_EN_REG, 0x0);
	i = 0;
	do {
		udelay(10);
		rv = x2efuse_rd(X2_EFUSE_WRAP_DONW_REG);
		i++;
	} while ((!(rv&0x1)) && (i < X2_EFUSE_RETRYS));
	if (i >= X2_EFUSE_RETRYS) {
		printf("wrap read operate timeout!\n");
		rv = readl((void *)X2_SWRST_REG_BASE);
		rv |= X2_EFUSE_OP_REPAIR_RST;
		writel(rv, (void *)X2_SWRST_REG_BASE);
		return -EIO;
	}
	*val = x2efuse_rd(X2_EFUSE_WRAP_DATA_BASE+(word*4));
	rv = readl((void *)X2_SWRST_REG_BASE);
	rv |= X2_EFUSE_OP_REPAIR_RST;
	writel(rv, (void *)X2_SWRST_REG_BASE);
	return 0;
}
#endif

static int hb_set_socuid(int offset)
{
	int  len;		/* new length of the property */
	int  ret;		/* return value */
	const void *ptmp;
	char *prop = "socuid";
	static char node_data[SCRATCHPAD] __aligned(4);/* property storage */

	ptmp = fdt_getprop(hb_dtb, offset, prop, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}

	if (ptmp != NULL)
		memcpy(node_data, ptmp, len);

	memset(node_data, 0, sizeof(node_data));
	snprintf(node_data, sizeof(node_data), "0x");
	ret = hb_get_socuid(node_data + strlen(node_data));
	if(ret < 0) {
		printf("get_socuid error\n");
		return 1;
	}

	len = strlen(node_data) + 1;
	ret = fdt_setprop(hb_dtb, offset, prop, node_data, len);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return 0;
}

static int find_fdt(unsigned long fdt_addr)
{
	struct fdt_header *blob;
	blob = map_sysmem(fdt_addr, 0);
	if (!valid_fdt(&blob))
		return 1;
	set_hb_fdt_addr(fdt_addr);
	return 0;
}

static int hb_dtb_property_config(int offset, char *prop, int value)
{
	int  len;		/* new length of the property */
	int  ret;		/* return value */
	static char node_data[SCRATCHPAD] __aligned(4);/* property storage */

	fdt_getprop(hb_dtb, offset, prop, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}

	snprintf(node_data, sizeof(node_data), "%x", value);
	len = strlen(node_data) + 1;

	ret = fdt_setprop(hb_dtb, offset, prop, node_data, len);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return 0;
}

static int hb_set_board_id(int offset)
{
	int  ret;		/* return value */
	char *prop   = "board_id";
	char *prop_origin = "origin_board_id";
	unsigned int board_id = 0;

	/* set origin board id */
	ret = hb_dtb_property_config(offset, prop_origin, x3_origin_board_id);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set boardid */
	board_id = ((x3_ddr_vender & 0xf) << 28) | (x3_ddr_type << 24) | \
		((x3_ddr_freq & 0xf) << 20) | ((x3_ddr_size & 0xf) << 16) | \
		((hb_som_type & 0xff) << 8) | (hb_base_board_type & 0xff);

	ret = hb_dtb_property_config(offset, prop, board_id);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

static int hb_set_boot_mode(int offset)
{
	int  ret;		/* return value */
	char *prop   = "boot_mode";
	unsigned int boot_mode = hb_boot_mode_get();

	ret = hb_dtb_property_config(offset, prop, boot_mode);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

static void hb_board_config_init(void) {
	uint32_t ddr_model, ddr_freq;
	uint32_t reg = reg32_read(X2_GPIO_BASE + X2_STRAP_PIN_REG);

	/* init base board type */
	hb_base_board_type = hb_base_board_type_get();

	/* init ddr vender */
	ddr_model = DDR_MANUF_SEL(hb_board_id);
	if (!ddr_model) {
		/* use strap pin to decide ddr model */
		if (PIN_DDR_TYPE_SEL(reg) == 0)
			ddr_model = DDR_MANU_HYNIX;
		else
			ddr_model = DDR_MANU_MICRON;
	}
	x3_ddr_vender = ddr_model;

	/* init ddr type */
	x3_ddr_type = DDR_TYPE_SEL(hb_board_id);

	/* init ddr size */
	x3_ddr_size = DDR_CAPACITY_SEL(hb_board_id);

	/* init ddr freq */
	ddr_freq = DDR_FREQ_SEL(hb_board_id);
	if (!ddr_freq) {
		x3_ddr_freq = DDR_FREQC_2666;
	} else {
		x3_ddr_freq = ddr_freq;
	}
}

static int hb_set_ddr_property(int offset)
{
	int  ret;
	char *prop_type = "ddr_type";
	char *prop_size = "ddr_size";
	char *prop_vender = "ddr_vender";
	char *prop_freq = "ddr_freq";
	char *prop_part_num = "ddr_part_num";

	/* set ddr_vender */
	ret = hb_dtb_property_config(offset, prop_vender, x3_ddr_vender);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr_size */
	ret = hb_dtb_property_config(offset, prop_size, x3_ddr_size);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr_type */
	ret = hb_dtb_property_config(offset, prop_type, x3_ddr_type);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr_freq */
	ret = hb_dtb_property_config(offset, prop_freq, x3_ddr_freq);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr_part_num */
	ret = hb_dtb_property_config(offset, prop_part_num, x3_ddr_part_num);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

static int hb_set_board_type(int offset)
{
	int  ret;
	char *prop_som_type = "som_name";
	char *prop_base_board_type = "base_board_name";

	/* set ddr_vender */
	ret = hb_dtb_property_config(offset, prop_som_type, hb_som_type);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr_vender */
	ret = hb_dtb_property_config(offset, prop_base_board_type,
		hb_base_board_type);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

static int hb_set_board_chip_id(int offset)
{
	int  ret;
	char *prop_chip_id = "chip_id";
	int chip_id;

	chip_id = reg32_read(SEC_REG_BASE + 0x70);
	printf("chip_id: %x\n", chip_id);
	ret = hb_dtb_property_config(offset, prop_chip_id, chip_id);
	if (ret < 0) {
		printf("hb_dtb_property_config: %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

static int do_set_boardid(cmd_tbl_t *cmdtp, int flag,
		int argc, char *const argv[])
{
	int  nodeoffset;	/* node offset from libfdt */
	int  ret;		/* return value */
	char *pathp  = "/soc/socinfo";
	uint64_t dtb_addr;

	/* init property */
	hb_board_config_init();

	dtb_addr = env_get_ulong("fdt_addr", 16, FDT_ADDR);
	ret = find_fdt(dtb_addr);
	if(1 == ret) {
		printf("find_fdt error\n");
		return 1;
	}

	nodeoffset = fdt_path_offset(hb_dtb, pathp);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(nodeoffset));
		return 1;
	}

	/* set bootmode */
	ret = hb_set_boot_mode(nodeoffset);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set socuid */
	ret = hb_set_socuid(nodeoffset);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set ddr size, ddr freq, ddr vender and ddr type */
	ret = hb_set_ddr_property(nodeoffset);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set som type and base board type */
	ret = hb_set_board_type(nodeoffset);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set board id and origin board id */
	ret = hb_set_board_id(nodeoffset);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	ret = hb_set_board_chip_id(nodeoffset);
	if (ret < 0) {
		printf("hb_set_board_chip_id: %s\n", fdt_strerror(ret));
		return 1;
	}

	return ret;
}

U_BOOT_CMD(
		send_id, CONFIG_SYS_MAXARGS, 0, do_set_boardid,
		"send_boardid",
		"send board information to kernel by DTB");

