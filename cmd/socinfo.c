#include <linux/ctype.h>
#include <linux/types.h>
#include <common.h>
#include <mapmem.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <fdt_support.h>

#include "socinfo.h"
#include "../arch/arm/cpu/armv8/x2/x2_info.h"

#define SCRATCHPAD	1024

static int find_fdt(unsigned long fdt_addr);
static int valid_fdt(struct fdt_header **blobp);

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

static int find_fdt(unsigned long fdt_addr)
{
	struct fdt_header *blob;
	blob = map_sysmem(fdt_addr, 0);
	if (!valid_fdt(&blob))
		return 1;
	set_x2_fdt_addr(fdt_addr);
	return 0;
}

static int do_set_boardid(int argc, char *const argv[])
{
	int  nodeoffset;	/* node offset from libfdt */
	static char data[SCRATCHPAD] __aligned(4);/* property storage */
	const void *ptmp;
	int  len;		/* new length of the property */
	int  ret;		/* return value */
	char *pathp  = "/soc/socinfo";
	char *prop   = "board_id";
	unsigned int board_id;
	struct x2_info_hdr *x2_board_type;
	unsigned long dtb_addr = 0x4000000;
	
	x2_board_type = (struct x2_info_hdr *) X2_BOOTINFO_ADDR;
	board_id = x2_board_type ->board_id;
	
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

	ptmp = fdt_getprop(x2_dtb, nodeoffset, prop, &len);
	if (len > SCRATCHPAD) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
		       len);
		return 1;
	}

	if (ptmp != NULL)
		memcpy(data, ptmp, len);

	sprintf(data, "%02x", board_id);

	len = strlen(data) + 1;
	
	ret = fdt_setprop(x2_dtb, nodeoffset, prop, data, len);
	if (ret < 0) {
		printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}
	return 0;
}

U_BOOT_CMD(
	send_id, CONFIG_SYS_MAXARGS, 0, do_set_boardid,
	"send_boardid",
	"send boardid to kernel by DTB"
);
