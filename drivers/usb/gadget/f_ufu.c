// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * f_ufu.c -- TCL USB Firmware Upgrade Composite Function
 *
 * Copyright (C) 2020 Horizon Robotics
 *                    Author: Jianghe Xu<jianghe.xu@horizon.ai>
 * All rights reserved.
 */

#include <ufu.h>
#include <blk.h>
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <u-boot/md5.h>
#include <hexdump.h>

static struct usb_interface_descriptor ufu_intf_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= USB_CLASS_MASS_STORAGE,
	.bInterfaceSubClass	= USB_SC_SCSI,
	.bInterfaceProtocol	= USB_PR_BULK,
};

static struct usb_descriptor_header *ufu_fs_function[] = {
	(struct usb_descriptor_header *)&ufu_intf_desc,
	(struct usb_descriptor_header *)&fsg_fs_bulk_in_desc,
	(struct usb_descriptor_header *)&fsg_fs_bulk_out_desc,
	NULL,
};

static struct usb_descriptor_header *ufu_hs_function[] = {
	(struct usb_descriptor_header *)&ufu_intf_desc,
	(struct usb_descriptor_header *)&fsg_hs_bulk_in_desc,
	(struct usb_descriptor_header *)&fsg_hs_bulk_out_desc,
	NULL,
};

struct firmware_dispatch_info {
	enum ufu_firmware_subcmd cmd;
	/* call back function to handle ufu firmware upgrade command */
	int (*cb)(struct fsg_common *common, struct fsg_buffhd *bh);
};

static struct f_ufu *ufu_func;

__maybe_unused
static inline void dump_cbw(struct fsg_bulk_cb_wrap *cbw)
{
	assert(!cbw);

	debug("%s:\n", __func__);
	debug("Signature %x\n", cbw->Signature);
	debug("Tag %x\n", cbw->Tag);
	debug("DataTransferLength %x\n", cbw->DataTransferLength);
	debug("Flags %x\n", cbw->Flags);
	debug("LUN %x\n", cbw->Lun);
	debug("Length %x\n", cbw->Length);
	debug("OptionCode %x\n", cbw->CDB[0]);
	debug("SubCode %x\n", cbw->CDB[1]);
	debug("SectorAddr %x\n", get_unaligned_be32(&cbw->CDB[2]));
	debug("BlkSectors %x\n\n", get_unaligned_be16(&cbw->CDB[7]));
}

__maybe_unused
static inline void dump_loadinfo(struct load_info *info)
{
	assert(!info);
#if 0
	debug("%s:\n", __func__);
	debug("addr %x"\n, info->addr);
	debug("size %x"\n, info->addr);
#else
	printf("%s:\n", __func__);
	printf("addr 0x%x\n", info->addr);
	printf("size 0x%x\n", info->size);
	print_hex_dump("md5: ", DUMP_PREFIX_NONE, 32, 1,
			info->md5, 16, 1);
#endif
}

static int ufu_check_lun(struct fsg_common *common)
{
	struct fsg_lun *curlun;

	/* Check the LUN */
	if (common->lun >= 0 && common->lun < common->nluns) {
		curlun = &common->luns[common->lun];
		if (common->cmnd[0] != SC_REQUEST_SENSE) {
			curlun->sense_data = SS_NO_SENSE;
			curlun->info_valid = 0;
		}
	} else {
		curlun = NULL;
		common->bad_lun_okay = 0;

		/*
		 * INQUIRY and REQUEST SENSE commands are explicitly allowed
		 * to use unsupported LUNs; all others may not.
		 */
		if (common->cmnd[0] != SC_INQUIRY &&
		    common->cmnd[0] != SC_REQUEST_SENSE) {
			debug("unsupported LUN %d\n", common->lun);
			return -EINVAL;
		}
	}

	return 0;
}

static int md5sum_check(void)
{
	struct f_ufu		*f_ufu	= get_ufu();
	unsigned char		md5[16];

	void *addr = (void *)((u64)(f_ufu->loadinfo.addr));
	md5_wd(addr, f_ufu->loadinfo.size, md5, CHUNKSZ_MD5);

	print_hex_dump("md5: ", DUMP_PREFIX_NONE, 32, 1,
			md5, 16, 1);
	if (memcmp(md5, f_ufu->loadinfo.md5, 16)) {
		printf("md5 mismatch\n");
		print_hex_dump("expect md5: ", DUMP_PREFIX_NONE, 32, 1,
				f_ufu->loadinfo.md5, 16, 1);
		print_hex_dump("actual md5: ", DUMP_PREFIX_NONE, 32, 1,
				md5, 16, 1);

		return -EFAULT;
	}

	return 0;
}

/**
 * ufu_mmc_blk_write() - Write/erase MMC in chunks of UFU_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t ufu_mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += UFU_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, UFU_MAX_BLK_WRITE);
		if (buffer)
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		else
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		blk += blks_written;
		blks += blks_written;
	}

	return blks;
}

/**
 * write_raw_image_to_addr - write raw image to addr
 */
static int write_raw_image_to_addr(struct blk_desc *dev_desc, long addr,
		int blksz, void *buffer, u32 download_bytes)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (blksz - 1)) & ~(blksz - 1));
	blkcnt = lldiv(blkcnt, blksz);

	puts("Flashing Raw Image\n");

	blks = ufu_mmc_blk_write(dev_desc, addr, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed writing to device %d\n", dev_desc->devnum);
		return -EFAULT;
	}

	printf("........ wrote " LBAFU " bytes to 0x%lx\n",
			blkcnt * blksz, addr);

	return 0;
}

/**
 * ufu_mmc_flash_write() - Write image to eMMC for ufu
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
static int ufu_mmc_flash_write(void *download_buffer, u32 download_bytes)
{
	struct blk_desc *dev_desc;
	long start_addr = 0x0;
	int r;

	dev_desc = blk_get_dev("mmc", 0);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return -EINVAL;
	}

	r = write_raw_image_to_addr(dev_desc, start_addr,
			dev_desc->blksz, download_buffer,
			download_bytes);

	return r;
}

static int do_flash(void)
{
	struct f_ufu		*f_ufu	= get_ufu();
	int r;

	void	*download_image = (void *)((u64)(f_ufu->loadinfo.addr));
	u32	size = f_ufu->loadinfo.size;

	/* if (emmc) */
	r = ufu_mmc_flash_write(download_image, size);

	return r;
}

static int cb_download_keep(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	struct f_ufu		*f_ufu	= get_ufu();
	struct fsg_buffhd	*next_bh;
	int rc;

	if (common->data_size > FSG_BUFLEN &&
			common->data_dir != DATA_DIR_FROM_HOST) {
		printf("data_size(%u) must be less than FSG_BUFLEN(%u), "
				"data_dir(%u) must be DATA_DIR_FROM_HOST\n",
				common->data_size, FSG_BUFLEN,
				common->data_dir);
		return -EINVAL;
	}

	if (f_ufu->loadinfo.addr < 0x6000000) {
		pr_err("<%s> 0 ~ 96M(0x6000000) is uboot code/stack/heap. "
				"make sure address bigger than 96M. \n",
				__func__);
		return -EIO;
	}

	common->residue		= common->data_size;
	common->usb_amount_left = common->data_size;

	for (;;) {
		if (common->usb_amount_left > 0) {
			/* Wait for the next buffer to become available */
			next_bh = common->next_buffhd_to_fill;
			if (next_bh->state != BUF_STATE_EMPTY)
				goto wait;

			/* Request the next buffer */
			common->usb_amount_left		-= common->data_size;
			next_bh->outreq->length		= common->data_size;
			next_bh->bulk_out_intended_length	= common->data_size;
			next_bh->outreq->short_not_ok	= 1;

			START_TRANSFER_OR(common, bulk_out, next_bh->outreq,
					&next_bh->outreq_busy, &next_bh->state)
				/*
				 * Don't know what to do
				 * if common->fsg is NULL
				 */
				return -EIO;
		} else {
			/* Then, wait for the data to become available */
			next_bh = common->next_buffhd_to_drain;
			if (next_bh->state != BUF_STATE_FULL)
				goto wait;

			common->next_buffhd_to_drain = next_bh->next;
			next_bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (next_bh->outreq->status != 0)
				break;

			/* Save the flash image data to ddr(addr + offset) */
			void *target = (void *)((u64)(f_ufu->loadinfo.addr + f_ufu->offset));
			memcpy(target, next_bh->buf, common->data_size);

			f_ufu->offset += common->data_size;
			f_ufu->chunks++;

			common->residue -= common->data_size;

			/* Dis the host decide to stop early? */
			if (next_bh->outreq->actual != next_bh->outreq->length)
				common->short_packet_received = 1;

			if (!(f_ufu->chunks % 3))
				putc('.');

			if (!(f_ufu->chunks % (74 * 3)))
				putc('\n');

			break;	/* Command done */
		}
wait:
		/* Wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return 0;
}

static int cb_get_result(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	struct f_ufu	*f_ufu	= get_ufu();
	u8		*buf = (u8 *)bh->buf;
	u32		len = common->data_size;

	if (common->data_size < 4 &&
			common->data_dir != DATA_DIR_TO_HOST) {
		printf("common data_size(%u) must > 4, "
			"and data_dir(%u) must be DATA_DIR_TO_HOST\n",
			common->data_size, common->data_dir);
		return -EINVAL;
	}

	buf[0] = 0x0d;
	if (f_ufu->download_finish && f_ufu->md5_pass &&
			f_ufu->flash_finish)
		buf[1] = UFU_SUCCESS;		/* download, md5 & flash ok */
	else if (!f_ufu->md5_pass)
		buf[1] = UFU_ERR_MD5;		/* md5 fail */
	else
		buf[1] = UFU_ERR_MAX;		/* unknown error*/

	buf[2] = 0x00;
	buf[3] = 0x00;

	/* Set data xfer size */
	common->residue = common->data_size_from_cmnd = len;

	return len;
}

static int cb_get_state(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	return 0;
}

static int cb_download_end(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	struct f_ufu		*f_ufu	= get_ufu();
	struct fsg_buffhd	*next_bh;
	int rc;

	putc('\n');

	if (common->data_size > FSG_BUFLEN &&
			common->data_dir != DATA_DIR_FROM_HOST) {
		printf("data_size(%u) must be less than FSG_BUFLEN(%u), "
				"data_dir(%u) must be DATA_DIR_FROM_HOST\n",
				common->data_size, FSG_BUFLEN,
				common->data_dir);
		return -EINVAL;
	}

	if (f_ufu->loadinfo.addr < 0x6000000) {
		pr_err("<%s> 0 ~ 96M(0x6000000) is uboot code/stack/heap"
				"make sure address bigger than 96M. \n",
				__func__);
		return -EIO;
	}

	common->residue		= common->data_size;
	common->usb_amount_left = common->data_size;

	for (;;) {
		if (common->usb_amount_left > 0) {
			/* Wait for the next buffer to become available */
			next_bh = common->next_buffhd_to_fill;
			if (next_bh->state != BUF_STATE_EMPTY)
				goto wait;

			/* Request the next buffer */
			common->usb_amount_left		-= common->data_size;
			next_bh->outreq->length		= common->data_size;
			next_bh->bulk_out_intended_length	= common->data_size;
			next_bh->outreq->short_not_ok	= 1;

			START_TRANSFER_OR(common, bulk_out, next_bh->outreq,
					&next_bh->outreq_busy, &next_bh->state)
				/*
				 * Don't know what to do
				 * if common->fsg is NULL
				 */
				return -EIO;
		} else {
			/* Then, wait for the data to become available */
			next_bh = common->next_buffhd_to_drain;
			if (next_bh->state != BUF_STATE_FULL)
				goto wait;

			common->next_buffhd_to_drain = next_bh->next;
			next_bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (next_bh->outreq->status != 0)
				break;

			/* Save the flash image data to ddr(addr + offset) */
			void *target = (void *)((u64)(f_ufu->loadinfo.addr + f_ufu->offset));
			memcpy(target, next_bh->buf, common->data_size);

			f_ufu->offset += common->data_size;
			f_ufu->chunks++;

			if (f_ufu->offset != f_ufu->loadinfo.size) {
				pr_err("some error happen!! offset(0x%x) "
						"!= loadinfo.size(0x%x)\n",
						f_ufu->offset,
						f_ufu->loadinfo.size);
				return -EIO;
			}

			common->residue -= common->data_size;

			/* Dis the host decide to stop early? */
			if (next_bh->outreq->actual != next_bh->outreq->length)
				common->short_packet_received = 1;

			f_ufu->download_finish = 1;
			printf("download end...\n");

			if (md5sum_check() < 0)
				f_ufu->md5_pass = 0;
			else
				f_ufu->md5_pass = 1;

			printf("%s\n", f_ufu->md5_pass ? "md5 checksum pass"
					: "md5 checksum fail");

			if (f_ufu->md5_pass)
				f_ufu->flash_finish = do_flash() ? 0 : 1;

			printf("%s\n", f_ufu->flash_finish ? "flash succeed"
					: "flash failed");

			break;	/* Command done */
		}
wait:
		/* Wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return 0;
}

static int cb_loadinfo(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	struct f_ufu		*f_ufu	= get_ufu();
	struct fsg_buffhd	*next_bh;
	struct load_info	*info;
	int rc;

	if (common->data_size != 24 &&
			common->data_dir != DATA_DIR_FROM_HOST) {
		printf("common data_size(%u) must be 24, "
			"and data_dir(%u) must be DATA_DIR_FROM_HOST\n",
			common->data_size, common->data_dir);
		return -EINVAL;
	}

	common->residue		= common->data_size;
	common->usb_amount_left = common->data_size;

	for (;;) {
		if (common->usb_amount_left > 0) {
			/* Wait for the next buffer to become available */
			next_bh = common->next_buffhd_to_fill;
			if (next_bh->state != BUF_STATE_EMPTY)
				goto wait;

			/* Request the next buffer */
			common->usb_amount_left		-= common->data_size;
			next_bh->outreq->length		= common->data_size;
			next_bh->bulk_out_intended_length	= common->data_size;
			next_bh->outreq->short_not_ok	= 1;

			START_TRANSFER_OR(common, bulk_out, next_bh->outreq,
					&next_bh->outreq_busy, &next_bh->state)
				/*
				 * Don't know what to do
				 * if common->fsg is NULL
				 */
				return -EIO;
		} else {
			/* Then, wait for the data to become available */
			next_bh = common->next_buffhd_to_drain;
			if (next_bh->state != BUF_STATE_FULL)
				goto wait;

			common->next_buffhd_to_drain = next_bh->next;
			next_bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (next_bh->outreq->status != 0)
				break;

			/* Get the Real data */
			info = (struct load_info *)next_bh->buf;
			f_ufu->loadinfo = *info;

			dump_loadinfo(&f_ufu->loadinfo);

			if (info->addr < 0x6000000) {
				printf("<%s> 0 ~ 96M(0x6000000) is uboot code/stack/heap"
						"make sure address bigger than 96M. \n",
						__func__);
				return -EIO;
			}

			common->residue -= common->data_size;

			/* Dis the host decide to stop early? */
			if (next_bh->outreq->actual != next_bh->outreq->length)
				common->short_packet_received = 1;
			break;	/* Command done */
		}
wait:
		/* Wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return 0;
}

static void ufu_run_host_command(void)
{
	struct f_ufu		*f_ufu	= get_ufu();

	if (!strlen(f_ufu->command))
		return;

	if (run_command(f_ufu->command, 0))
		printf("run_commnad [%s] failed\n", f_ufu->command);
	else
		printf("run_commnad [%s] succeed\n", f_ufu->command);

	/* reset command from host */
	memset(f_ufu->command, 0, UFU_RUN_COMMNAD_MAX_LENGTH);
}

static int cb_run(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	struct f_ufu		*f_ufu	= get_ufu();
	struct fsg_buffhd	*next_bh;
	int rc;

	if (common->data_size >= UFU_RUN_COMMNAD_MAX_LENGTH ||
			common->data_dir != DATA_DIR_FROM_HOST) {
		printf("common length(%u) exceed, must be less than %u, "
			"and data_dir(%u) must be DATA_DIR_FROM_HOST\n",
			common->data_size, UFU_RUN_COMMNAD_MAX_LENGTH,
			common->data_dir);
		return -EINVAL;
	}

	common->residue		= common->data_size;
	common->usb_amount_left = common->data_size;

	for (;;) {
		if (common->usb_amount_left > 0) {
			/* Wait for the next buffer to become available */
			next_bh = common->next_buffhd_to_fill;
			if (next_bh->state != BUF_STATE_EMPTY)
				goto wait;

			/* Request the next buffer */
			common->usb_amount_left		-= common->data_size;
			next_bh->outreq->length		= common->data_size;
			next_bh->bulk_out_intended_length	= common->data_size;
			next_bh->outreq->short_not_ok	= 1;

			START_TRANSFER_OR(common, bulk_out, next_bh->outreq,
					&next_bh->outreq_busy, &next_bh->state)
				/*
				 * Don't know what to do
				 * if common->fsg is NULL
				 */
				return -EIO;
		} else {
			/* Then, wait for the data to become available */
			next_bh = common->next_buffhd_to_drain;
			if (next_bh->state != BUF_STATE_FULL)
				goto wait;

			common->next_buffhd_to_drain = next_bh->next;
			next_bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (next_bh->outreq->status != 0)
				break;

			/* Get the command from host */
			strncpy(f_ufu->command, next_bh->buf, common->data_size);
			f_ufu->command[common->data_size] = '\0';

			common->residue -= common->data_size;

			/* Dis the host decide to stop early? */
			if (next_bh->outreq->actual != next_bh->outreq->length)
				common->short_packet_received = 1;
			break;	/* Command done */
		}
wait:
		/* Wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return 0;
}

static int cb_identify(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	return 0;
}

static const struct firmware_dispatch_info firmware_dispatch_info[] = {
	{
		.cmd = UFU_SUBCMD_DOWNLOAD_KEEP,
		.cb = cb_download_keep,
	},
	{
		.cmd = UFU_SUBCMD_GET_RESULT,
		.cb = cb_get_result,
	},
	{
		.cmd = UFU_SUBCMD_GET_STATE,
		.cb = cb_get_state,
	},
	{
		.cmd = UFU_SUBCMD_DOWNLOAD_END,
		.cb = cb_download_end,
	},
	{
		.cmd = UFU_SUBCMD_LOADINFO,
		.cb = cb_loadinfo,
	},
	{
		.cmd = UFU_SUBCMD_RUN,
		.cb = cb_run,
	},
	{
		.cmd = UFU_SUBCMD_IDENTIFY,
		.cb = cb_identify,
	},
};

static int ufu_do_firmware_upgrade(struct fsg_common *common,
				struct fsg_buffhd *bh)
{
	int (*func_cb)(struct fsg_common *common, struct fsg_buffhd *bh) = NULL;
	u8 sub_command = common->cmnd[1];
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(firmware_dispatch_info); i++) {
		if (firmware_dispatch_info[i].cmd == sub_command) {
			func_cb = firmware_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		printf("firmware unknown sub-command: %u\n", sub_command);
		rc = -EINVAL;
	} else {
		rc = func_cb(common, bh);
	}

	return rc;
}

static int ufu_cmd_process(struct fsg_common *common,
			     struct fsg_buffhd *bh, int *reply)
{
	struct usb_request	*req = bh->outreq;
	struct fsg_bulk_cb_wrap	*cbw = req->buf;
	int rc;

	dump_cbw(cbw);

	if (ufu_check_lun(common)) {
		*reply = -EINVAL;
		return UFU_RC_ERROR;
	}

	switch (common->cmnd[0]) {
	case UFU_CMD_FIRMWARE:
		*reply = ufu_do_firmware_upgrade(common, bh);
		rc = UFU_RC_FINISHED;
		break;
	default:
		rc = UFU_RC_UNKNOWN_CMND;
		break;
	}

	return rc;
}

struct f_ufu *get_ufu(void)
{
	struct f_ufu *f_ufu = ufu_func;

	if (!f_ufu) {
		f_ufu = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_ufu));
		if (!f_ufu)
			return 0;

		ufu_func = f_ufu;
		memset(f_ufu, 0, sizeof(*f_ufu));
	}

	return f_ufu;
}

static int ufu_func_init(void)
{
	struct f_ufu *f_ufu = get_ufu();

	if (f_ufu)
		return 0;
	else
		return -EFAULT;
}

DECLARE_GADGET_BIND_CALLBACK(ufu_ums_dnl, fsg_add);
