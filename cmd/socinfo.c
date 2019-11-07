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

#include "socinfo.h"
#include "../arch/arm/cpu/armv8/x2/x2_info.h"

#define SCRATCHPAD	1024

static int find_fdt(unsigned long fdt_addr);
static int valid_fdt(struct fdt_header **blobp);
extern unsigned int x2_src_boot;

void set_x2_fdt_addr(ulong addr)
{
	void *buf;

	buf = map_sysmem(addr, 0);
	x2_dtb = buf;
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

static int get_socuid(char *socuid)
{
	char temp[8] = {0};
	u32 ret, val, word;
	for(word = 0; word < 4; word++) {
		ret = socuid_read(word, &val);
		if(ret)
			return -1;
		sprintf(temp, "%.8x", val);
		strcat(socuid, temp);
	}
	return 0;
}

static int find_fdt(unsigned long fdt_addr)
{
	struct fdt_header *blob;
	blob = map_sysmem(fdt_addr, 0);
	if (!valid_fdt(&blob))
		return 1;
	set_x2_fdt_addr(fdt_addr);
	return 0;
}

static int do_set_boardid(cmd_tbl_t *cmdtp, int flag,
		int argc, char *const argv[])
{
	int  nodeoffset;	/* node offset from libfdt */
	static char data[SCRATCHPAD] __aligned(4);/* property storage */
	const void *ptmp;
	int  len;		/* new length of the property */
	int  ret;		/* return value */
	char *pathp  = "/soc/socinfo";
	char *prop   = "board_id";
	char *prop_boot = "boot_mode";
	char *prop_socuid = "socuid";
	unsigned int board_id;
	struct x2_info_hdr *x2_board_type;
	unsigned long dtb_addr;

	x2_board_type = (struct x2_info_hdr *) X2_BOOTINFO_ADDR;
	board_id = x2_board_type ->board_id;

	dtb_addr = env_get_ulong("fdt_addr", 16, FDT_ADDR);
	ret = find_fdt(dtb_addr);
	if(1 == ret){
		printf("find_fdt error\n");
		return 1;
	}

	nodeoffset = fdt_path_offset (x2_dtb, pathp);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf ("libfdt fdt_path_offset() returned %s\n",
				fdt_strerror(nodeoffset));
		return 1;
	}

	/* set boardid */
	ptmp = fdt_getprop(x2_dtb, nodeoffset, prop, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}

	if (ptmp != NULL)
		memcpy(data, ptmp, len);

	sprintf(data, "%02x", board_id);
	printf("socinfo data = %s\n", data);

	len = strlen(data) + 1;

	ret = fdt_setprop(x2_dtb, nodeoffset, prop, data, len);
	if (ret < 0) {
		printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set bootmode */
	ptmp = fdt_getprop(x2_dtb, nodeoffset, prop_boot, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}

	if (ptmp != NULL)
		memcpy(data, ptmp, len);

	sprintf(data, "%d", x2_src_boot);
	len = strlen(data) + 1;

	ret = fdt_setprop(x2_dtb, nodeoffset, prop_boot, data, len);
	if (ret < 0) {
		printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	/* set socuid */
	ptmp = fdt_getprop(x2_dtb, nodeoffset, prop_socuid, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}

	memset(data, 0, sizeof(data));
	ret = get_socuid(data);
	if(ret < 0) {
		printf("get_socuid error\n");
		return 1;
	}

	len = strlen(data) + 1;
	ret = fdt_setprop(x2_dtb, nodeoffset, prop_socuid, data, len);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
		send_id, CONFIG_SYS_MAXARGS, 0, do_set_boardid,
		"send_boardid",
		"send board information to kernel by DTB");
