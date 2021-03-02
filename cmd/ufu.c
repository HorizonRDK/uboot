// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * cmd_ufu.c -- ufu command (TCL Usb Firmware Upgrade)
 *
 * Copyright (C) 2020 Horizon Robotics
 *                    Author: Jianghe Xu<jianghe.xu@horizon.ai>
 * All rights reserved.
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <console.h>
#include <g_dnl.h>
#include <part.h>
#include <usb.h>
#include <usb_mass_storage.h>
#include <ufu.h>

static struct ufu_device ufu;
static struct ufu_device *g_ufu;

static int ums_read_sector(struct ums *ums_dev,
			   ulong start, lbaint_t blkcnt, void *buf)
{
	struct blk_desc *block_dev = &ums_dev->block_dev;
	lbaint_t blkstart = start + ums_dev->start_sector;

	return blk_dread(block_dev, blkstart, blkcnt, buf);
}

static int ums_write_sector(struct ums *ums_dev,
			    ulong start, lbaint_t blkcnt, const void *buf)
{
	struct blk_desc *block_dev = &ums_dev->block_dev;
	lbaint_t blkstart = start + ums_dev->start_sector;

	return blk_dwrite(block_dev, blkstart, blkcnt, buf);
}

static void ufu_fini(void)
{
	int i;

	for (i = 0; i < g_ufu->ums_cnt; i++)
		free((void *)g_ufu->ums[i].name);

	free(g_ufu->ums);
	g_ufu->ums = NULL;
	g_ufu->ums_cnt = 0;
	g_ufu = NULL;
}

#define UFU_NAME_LEN 16

static int ufu_init(const char *devtype, const char *devnums_part_str)
{
	char *s, *t, *devnum_part_str, *name;
	struct blk_desc *block_dev;
	disk_partition_t info;
	int partnum, cnt;
	int ret = -1;
	struct ums *ums_new;

	s = strdup(devnums_part_str);
	if (!s)
		return -1;

	t = s;
	g_ufu->ums_cnt = 0;

	for (;;) {
		devnum_part_str = strsep(&t, ",");
		if (!devnum_part_str)
			break;

		partnum = blk_get_device_part_str(devtype, devnum_part_str,
					&block_dev, &info, 1);
		if (partnum < 0)
			goto cleanup;

		/* Check if the argument is in legacy format. If yes,
		 * expose all partitions by setting the partnum = 0
		 * e.g. ums 0 mmc 0
		 */
		if (!strchr(devnum_part_str, ':'))
			partnum = 0;

		/* f_mass_storage.c assumes SECTOR_SIZE sectors */
		if (block_dev->blksz != SECTOR_SIZE)
			goto cleanup;

		ums_new = realloc(g_ufu->ums, (g_ufu->ums_cnt + 1) *
				  sizeof(*g_ufu->ums));
		if (!ums_new)
			goto cleanup;
		g_ufu->ums = ums_new;
		cnt = g_ufu->ums_cnt;

		/* if partnum = 0, expose all partitions */
		if (partnum == 0) {
			g_ufu->ums[cnt].start_sector = 0;
			g_ufu->ums[cnt].num_sectors = block_dev->lba;
		} else {
			g_ufu->ums[cnt].start_sector = info.start;
			g_ufu->ums[cnt].num_sectors = info.size;
		}

		g_ufu->ums[cnt].read_sector = ums_read_sector;
		g_ufu->ums[cnt].write_sector = ums_write_sector;

		name = malloc(UFU_NAME_LEN);
		if (!name)
			goto cleanup;

		// Refer to ufu spec. UBOOT prefix means in uboot stage.
		snprintf(name, UFU_NAME_LEN, "UBOOT");
		g_ufu->ums[cnt].name = name;
		g_ufu->ums[cnt].block_dev = *block_dev;

		printf("UFU: LUN %d, dev %d, hwpart %d, sector %#x, count %#x. "
				"name \"%s\"\n",
		       g_ufu->ums_cnt,
		       g_ufu->ums[cnt].block_dev.devnum,
		       g_ufu->ums[cnt].block_dev.hwpart,
		       g_ufu->ums[cnt].start_sector,
		       g_ufu->ums[cnt].num_sectors,
		       g_ufu->ums[cnt].name);

		g_ufu->ums_cnt++;
	}

	if (g_ufu->ums_cnt)
		ret = 0;

cleanup:
	free(s);
	if (ret < 0)
		ufu_fini();

	return ret;
}

static int do_ufu(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *usb_controller;
	const char *devtype;
	const char *devnum;
	unsigned int controller_index;
	int rc;
	int cable_ready_timeout __maybe_unused;
	const char *s;

	if (argc != 4)
		return CMD_RET_USAGE;

	usb_controller = argv[1];
	devtype = argv[2];
	devnum	= argv[3];

	if (!strcmp(devtype, "mmc") && !strcmp(devnum, "1")) {
		pr_err("Forbid to flash mmc 1(sdcard)\n");
		return CMD_RET_FAILURE;
	}

	g_ufu = &ufu;
	rc = ufu_init(devtype, devnum);
	if (rc < 0)
		return CMD_RET_FAILURE;

	controller_index = (unsigned int)(simple_strtoul(
				usb_controller,	NULL, 0));
	rc = usb_gadget_initialize(controller_index);
	if (rc) {
		pr_err("Couldn't init USB controller.");
		rc = CMD_RET_FAILURE;
		goto cleanup_ufu;
	}

	rc = fsg_init(g_ufu->ums, g_ufu->ums_cnt);
	if (rc) {
		pr_err("fsg_init failed");
		rc = CMD_RET_FAILURE;
		goto cleanup_board;
	}

	s = env_get("serial#");
	if (s) {
		char *sn = (char *)calloc(strlen(s) + 1, sizeof(char));
		char *sn_p = sn;

		if (!sn)
			goto cleanup_board;

		memcpy(sn, s, strlen(s));
		while (*sn_p) {
			if (*sn_p == '\\' || *sn_p == '/')
				*sn_p = '_';
			sn_p++;
		}

		g_dnl_set_serialnumber(sn);
		free(sn);
	}

	rc = g_dnl_register("ufu_ums_dnl");
	if (rc) {
		pr_err("g_dnl_register failed");
		rc = CMD_RET_FAILURE;
		goto cleanup_board;
	}

	/* Timeout unit: seconds */
	cable_ready_timeout = UMS_CABLE_READY_TIMEOUT;

	if (!g_dnl_board_usb_cable_connected()) {
		puts("Please connect USB cable.\n");

		while (!g_dnl_board_usb_cable_connected()) {
			if (ctrlc()) {
				puts("\rCTRL+C - Operation aborted.\n");
				rc = CMD_RET_SUCCESS;
				goto cleanup_register;
			}
			if (!cable_ready_timeout) {
				puts("\rUSB cable not detected.\nCommand exit.\n");
				rc = CMD_RET_SUCCESS;
				goto cleanup_register;
			}

			printf("\rAuto exit in: %.2d s.", cable_ready_timeout);
			mdelay(1000);
			cable_ready_timeout--;
		}
		puts("\r\n");
	}

	while (1) {
		usb_gadget_handle_interrupts(controller_index);

		rc = fsg_main_thread(NULL);
		if (rc) {
			/* Check I/O error */
			if (rc == -EIO)
				printf("\rCheck USB cable connection\n");

			/* Check CTRL+C */
			if (rc == -EPIPE)
				printf("\rCTRL+C - Operation aborted\n");

			rc = CMD_RET_SUCCESS;
			goto cleanup_register;
		}
	}

cleanup_register:
	g_dnl_unregister();
cleanup_board:
	usb_gadget_release(controller_index);
cleanup_ufu:
	ufu_fini();

	return rc;
}

U_BOOT_CMD(ufu, 4, 1, do_ufu,
		  "Use the UFU [USB Firmware Upgrade]",
		  "<USB_controller> <devtype> <dev[:part]>  e.g. ufu 0 mmc 0\n"
);
