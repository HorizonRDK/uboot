// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Broadcom Corporation.
 */

#include <config.h>
#include <common.h>
#include <blk.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <image-sparse.h>
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <linux/compat.h>
#include <android_image.h>

#define FASTBOOT_MAX_BLK_WRITE 16384
#define FASTBOOT_MAX_BLK_READ 16384

#define BOOT_PARTITION_NAME "boot"

struct fb_mmc_sparse {
	struct blk_desc	*dev_desc;
};

static int part_get_info_by_name_or_alias(struct blk_desc *dev_desc,
		const char *name, disk_partition_t *info)
{
	int ret;

	ret = part_get_info_by_name(dev_desc, name, info);
	if (ret < 0) {
		/* strlen("fastboot_partition_alias_") + 32(part_name) + 1 */
		char env_alias_name[25 + 32 + 1];
		char *aliased_part_name;

		/* check for alias */
		strcpy(env_alias_name, "fastboot_partition_alias_");
		strncat(env_alias_name, name, 32);
		aliased_part_name = env_get(env_alias_name);
		if (aliased_part_name != NULL)
			ret = part_get_info_by_name(dev_desc,
					aliased_part_name, info);
	}
	return ret;
}

/**
 * fb_mmc_blk_write() - Write/erase MMC in chunks of FASTBOOT_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t fb_mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int32_t i;

	for (i = 0; i < blkcnt; i += FASTBOOT_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, FASTBOOT_MAX_BLK_WRITE);
		if (buffer) {
			if (fastboot_progress_callback)
				fastboot_progress_callback("writing");
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		} else {
			if (fastboot_progress_callback)
				fastboot_progress_callback("erasing");
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}

static lbaint_t fb_mmc_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	struct fb_mmc_sparse *sparse = info->priv;
	struct blk_desc *dev_desc = sparse->dev_desc;

	return fb_mmc_blk_write(dev_desc, blk, blkcnt, buffer);
}

static lbaint_t fb_mmc_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

/**
 * fb_mmc_blk_read() - Read partition from MMC
 *
 * @block_dev: Pointer to block device
 * @start: First block to read
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for read and upload
 */
static lbaint_t fb_mmc_blk_read(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_read;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int32_t i;

	if (!buffer)
		return -1;

	for (i = 0; i < blkcnt; i += FASTBOOT_MAX_BLK_READ) {
		cur_blkcnt = min((int)blkcnt - i, FASTBOOT_MAX_BLK_READ);
		if (fastboot_progress_callback)
			fastboot_progress_callback("reading");
		blks_read = blk_dread(block_dev, blk, cur_blkcnt,
					  buffer + (i * block_dev->blksz));

		blk += blks_read;
		blks += blks_read;
	}
	return blks;
}

static void write_raw_image(struct blk_desc *dev_desc, disk_partition_t *info,
		const char *part_name, void *buffer,
		u32 download_bytes, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		pr_err("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition", response);
		return;
	}

	puts("Flashing Raw Image\n");

	blks = fb_mmc_blk_write(dev_desc, info->start, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed writing to device %d\n", dev_desc->devnum);
		fastboot_fail("failed writing to device", response);
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay(NULL, response);
}

/**
 * write_raw_image_to_addr - write raw image to addr
 */
static void write_raw_image_to_addr(struct blk_desc *dev_desc, long addr,
		int blksz, void *buffer, u32 download_bytes, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (blksz - 1)) & ~(blksz - 1));
	blkcnt = lldiv(blkcnt, blksz);

	puts("Flashing Raw Image\n");

	blks = fb_mmc_blk_write(dev_desc, addr, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed writing to device %d\n", dev_desc->devnum);
		fastboot_fail("failed writing to device", response);
		return;
	}

	printf("........ wrote " LBAFU " bytes to 0x%lx\n",
			blkcnt * blksz, addr);
	fastboot_okay(NULL, response);
}

static int64_t read_raw_image(struct blk_desc *dev_desc, disk_partition_t *info,
		const char *part_name, void *buffer,
		u64 size, s64 offset, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;
	lbaint_t blks_offset;

	/* determine number of blocks to write */
	blkcnt = ((size + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		pr_err("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition", response);
		return -1;
	}

	blks_offset = ((offset + (info->blksz - 1)) & ~(info->blksz - 1));
	blks_offset = lldiv(blks_offset, info->blksz);

	if (blkcnt + blks_offset > info->size) {
		pr_err("too large for partition: '%s'. blkcnt(%lu), blks_offset(%lu), size(%lu)\n",
				part_name, blkcnt, blks_offset, info->size);
		fastboot_fail("too large for partition", response);
		return -1;
	}

	puts("Loading Raw Image\n");

	blks = fb_mmc_blk_read(dev_desc, info->start + blks_offset, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed to read from device %d\n", dev_desc->devnum);
		fastboot_fail("failed to read from device", response);
		return blks * info->blksz;
	}

	printf("........ read " LBAFU " bytes from '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay(NULL, response);

	return blks * info->blksz;
}

/**
 * read_raw_image_to_addr - write raw image to addr
 */
static int64_t read_raw_image_from_addr(struct blk_desc *dev_desc, u64 addr,
		u64 blksz, void *buffer, u64 size, s64 offset, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;
	lbaint_t blks_offset;

	/* determine number of blocks to read */
	blkcnt = ((size + (blksz - 1)) & ~(blksz - 1));
	blkcnt = lldiv(blkcnt, blksz);

	blks_offset = ((offset + (blksz - 1)) & ~(blksz - 1));
	blks_offset = lldiv(blks_offset, blksz);

	puts("Flashing Raw Image\n");

	blks = fb_mmc_blk_read(dev_desc, addr + blks_offset, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed reading from device %d\n", dev_desc->devnum);
		fastboot_fail("failed reading from device", response);
		return -1;
	}

	printf("........ read \" %llu \" bytes from 0x%llx\n",
			blkcnt * blksz, addr + blks_offset);
	fastboot_okay(NULL, response);

	return blks * dev_desc->blksz;
}

#ifdef CONFIG_ANDROID_BOOT_IMAGE
/**
 * Read Android boot image header from boot partition.
 *
 * @param[in] dev_desc MMC device descriptor
 * @param[in] info Boot partition info
 * @param[out] hdr Where to store read boot image header
 *
 * @return Boot image header sectors count or 0 on error
 */
static lbaint_t fb_mmc_get_boot_header(struct blk_desc *dev_desc,
				       disk_partition_t *info,
				       struct andr_img_hdr *hdr,
				       char *response)
{
	ulong sector_size;		/* boot partition sector size */
	lbaint_t hdr_sectors;		/* boot image header sectors count */
	int res;

	/* Calculate boot image sectors count */
	sector_size = info->blksz;
	hdr_sectors = DIV_ROUND_UP(sizeof(struct andr_img_hdr), sector_size);
	if (hdr_sectors == 0) {
		pr_err("invalid number of boot sectors: 0\n");
		fastboot_fail("invalid number of boot sectors: 0", response);
		return 0;
	}

	/* Read the boot image header */
	res = blk_dread(dev_desc, info->start, hdr_sectors, (void *)hdr);
	if (res != hdr_sectors) {
		pr_err("cannot read header from boot partition\n");
		fastboot_fail("cannot read header from boot partition",
			      response);
		return 0;
	}

	/* Check boot header magic string */
	res = android_image_check_header(hdr);
	if (res != 0) {
		pr_err("bad boot image magic\n");
		fastboot_fail("boot partition not initialized", response);
		return 0;
	}

	return hdr_sectors;
}

/**
 * Write downloaded zImage to boot partition and repack it properly.
 *
 * @param dev_desc MMC device descriptor
 * @param download_buffer Address to fastboot buffer with zImage in it
 * @param download_bytes Size of fastboot buffer, in bytes
 *
 * @return 0 on success or -1 on error
 */
static int fb_mmc_update_zimage(struct blk_desc *dev_desc,
				void *download_buffer,
				u32 download_bytes,
				char *response)
{
	uintptr_t hdr_addr;			/* boot image header address */
	struct andr_img_hdr *hdr;		/* boot image header */
	lbaint_t hdr_sectors;			/* boot image header sectors */
	u8 *ramdisk_buffer;
	u32 ramdisk_sector_start;
	u32 ramdisk_sectors;
	u32 kernel_sector_start;
	u32 kernel_sectors;
	u32 sectors_per_page;
	disk_partition_t info;
	int res;

	puts("Flashing zImage\n");

	/* Get boot partition info */
	res = part_get_info_by_name(dev_desc, BOOT_PARTITION_NAME, &info);
	if (res < 0) {
		pr_err("cannot find boot partition\n");
		fastboot_fail("cannot find boot partition", response);
		return -1;
	}

	/* Put boot image header in fastboot buffer after downloaded zImage */
	hdr_addr = (uintptr_t)download_buffer + ALIGN(download_bytes, PAGE_SIZE);
	hdr = (struct andr_img_hdr *)hdr_addr;

	/* Read boot image header */
	hdr_sectors = fb_mmc_get_boot_header(dev_desc, &info, hdr, response);
	if (hdr_sectors == 0) {
		pr_err("unable to read boot image header\n");
		fastboot_fail("unable to read boot image header", response);
		return -1;
	}

	/* Check if boot image has second stage in it (we don't support it) */
	if (hdr->second_size > 0) {
		pr_err("moving second stage is not supported yet\n");
		fastboot_fail("moving second stage is not supported yet",
			      response);
		return -1;
	}

	/* Extract ramdisk location */
	sectors_per_page = hdr->page_size / info.blksz;
	ramdisk_sector_start = info.start + sectors_per_page;
	ramdisk_sector_start += DIV_ROUND_UP(hdr->kernel_size, hdr->page_size) *
					     sectors_per_page;
	ramdisk_sectors = DIV_ROUND_UP(hdr->ramdisk_size, hdr->page_size) *
				       sectors_per_page;

	/* Read ramdisk and put it in fastboot buffer after boot image header */
	ramdisk_buffer = (u8 *)hdr + (hdr_sectors * info.blksz);
	res = blk_dread(dev_desc, ramdisk_sector_start, ramdisk_sectors,
			ramdisk_buffer);
	if (res != ramdisk_sectors) {
		pr_err("cannot read ramdisk from boot partition\n");
		fastboot_fail("cannot read ramdisk from boot partition",
			      response);
		return -1;
	}

	/* Write new kernel size to boot image header */
	hdr->kernel_size = download_bytes;
	res = blk_dwrite(dev_desc, info.start, hdr_sectors, (void *)hdr);
	if (res == 0) {
		pr_err("cannot writeback boot image header\n");
		fastboot_fail("cannot write back boot image header", response);
		return -1;
	}

	/* Write the new downloaded kernel */
	kernel_sector_start = info.start + sectors_per_page;
	kernel_sectors = DIV_ROUND_UP(hdr->kernel_size, hdr->page_size) *
				      sectors_per_page;
	res = blk_dwrite(dev_desc, kernel_sector_start, kernel_sectors,
			 download_buffer);
	if (res == 0) {
		pr_err("cannot write new kernel\n");
		fastboot_fail("cannot write new kernel", response);
		return -1;
	}

	/* Write the saved ramdisk back */
	ramdisk_sector_start = info.start + sectors_per_page;
	ramdisk_sector_start += DIV_ROUND_UP(hdr->kernel_size, hdr->page_size) *
					     sectors_per_page;
	res = blk_dwrite(dev_desc, ramdisk_sector_start, ramdisk_sectors,
			 ramdisk_buffer);
	if (res == 0) {
		pr_err("cannot write back original ramdisk\n");
		fastboot_fail("cannot write back original ramdisk", response);
		return -1;
	}

	puts("........ zImage was updated in boot partition\n");
	fastboot_okay(NULL, response);
	return 0;
}
#endif

/**
 * fastboot_mmc_get_part_info() - Lookup eMMC partion by name
 *
 * @part_name: Named partition to lookup
 * @dev_desc: Pointer to returned blk_desc pointer
 * @part_info: Pointer to returned disk_partition_t
 * @response: Pointer to fastboot response buffer
 */
int fastboot_mmc_get_part_info(char *part_name, struct blk_desc **dev_desc,
			       disk_partition_t *part_info, char *response)
{
	int r;

	*dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!*dev_desc) {
		fastboot_fail("block device not found", response);
		return -ENOENT;
	}
	if (!part_name) {
		fastboot_fail("partition not found", response);
		return -ENOENT;
	}

	r = part_get_info_by_name_or_alias(*dev_desc, part_name, part_info);
	if (r < 0) {
		fastboot_fail("partition not found", response);

		return r;
	}

	return r;
}

/**
 * fastboot_mmc_flash_write() - Write image to eMMC for fastboot
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mmc_flash_write(const char *cmd, void *download_buffer,
			      u32 download_bytes, char *response)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	char cmdbuf[32];
	long start_addr = -1;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		fastboot_fail("invalid mmc device", response);
		return;
	}

#if CONFIG_IS_ENABLED(EFI_PARTITION)
	if (strcmp(cmd, CONFIG_FASTBOOT_GPT_NAME) == 0) {
		printf("%s: updating MBR, Primary and Backup GPT(s)\n",
		       __func__);
		if (is_valid_gpt_buf(dev_desc, download_buffer)) {
			printf("%s: invalid GPT - refusing to write to flash\n",
			       __func__);
			fastboot_fail("invalid GPT partition", response);
			return;
		}
		if (write_mbr_and_gpt_partitions(dev_desc, download_buffer)) {
			printf("%s: writing GPT partitions failed\n", __func__);
			fastboot_fail("writing GPT partitions failed",
				      response);
			return;
		}

#if CONFIG_IS_ENABLED(FASTBOOT_OEM_GPT_EXTEND)
		/* extend last partition size to maximum size */
		sprintf(cmdbuf, "gpt extend mmc %x",
			CONFIG_FASTBOOT_FLASH_MMC_DEV);
		printf("do '%s'\n", cmdbuf);
		run_command(cmdbuf, 0);
#endif
		printf("........ success\n");
		fastboot_okay(NULL, response);
		return;
	}
#endif

#if CONFIG_IS_ENABLED(DOS_PARTITION)
	if (strcmp(cmd, CONFIG_FASTBOOT_MBR_NAME) == 0) {
		printf("%s: updating MBR\n", __func__);
		if (is_valid_dos_buf(download_buffer)) {
			printf("%s: invalid MBR - refusing to write to flash\n",
			       __func__);
			fastboot_fail("invalid MBR partition", response);
			return;
		}
		if (write_mbr_partition(dev_desc, download_buffer)) {
			printf("%s: writing MBR partition failed\n", __func__);
			fastboot_fail("writing MBR partition failed",
				      response);
			return;
		}
		printf("........ success\n");
		fastboot_okay(NULL, response);
		return;
	}
#endif

#ifdef CONFIG_ANDROID_BOOT_IMAGE
	if (strncasecmp(cmd, "zimage", 6) == 0) {
		fb_mmc_update_zimage(dev_desc, download_buffer,
				     download_bytes, response);
		return;
	}
#endif

	if (part_get_info_by_name_or_alias(dev_desc, cmd, &info) < 0) {
		pr_err("cannot find partition: '%s'\n", cmd);
		fastboot_fail("cannot find partition", response);

		/* fallback on using the 'partition name' as a number */
		if (strict_strtoul(cmd, 16, (unsigned long *)&start_addr) < 0)
			return;

		printf("fastboot emmc flash start_addr: 0x%lx\n", start_addr);
	}

	if (is_sparse_image(download_buffer)) {
		struct fb_mmc_sparse sparse_priv;
		struct sparse_storage sparse;
		int err;

		sparse_priv.dev_desc = dev_desc;
		if (start_addr == -1) {
			sparse.blksz = info.blksz;
			sparse.start = info.start;
			sparse.size = info.size;
		} else {
			sparse.blksz = dev_desc->blksz;
			sparse.start = start_addr;
			sparse.size  = dev_desc->lba * dev_desc->blksz;
		}
		sparse.write = fb_mmc_sparse_write;
		sparse.reserve = fb_mmc_sparse_reserve;
		sparse.mssg = fastboot_fail;

		printf("Flashing sparse image at offset " LBAFU "\n",
		       sparse.start);

		sparse.priv = &sparse_priv;
		err = write_sparse_image(&sparse, cmd, download_buffer,
					 response);
		if (!err)
			fastboot_okay(NULL, response);
	} else {
		if (start_addr == -1)
			write_raw_image(dev_desc, &info, cmd, download_buffer,
					download_bytes, response);
		else
			write_raw_image_to_addr(dev_desc, start_addr,
					dev_desc->blksz, download_buffer,
					download_bytes, response);
	}
}

/**
 * fastboot_mmc_flash_erase() - Erase eMMC for fastboot
 *
 * @cmd: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mmc_erase(const char *cmd, char *response)
{
	int ret;
	struct blk_desc *dev_desc;
	disk_partition_t info;
	lbaint_t blks, blks_start, blks_size, grp_size;
	struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (mmc == NULL) {
		pr_err("invalid mmc device\n");
		fastboot_fail("invalid mmc device", response);
		return;
	}

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		fastboot_fail("invalid mmc device", response);
		return;
	}

	ret = part_get_info_by_name_or_alias(dev_desc, cmd, &info);
	if (ret < 0) {
		pr_err("cannot find partition: '%s'\n", cmd);
		fastboot_fail("cannot find partition", response);
		return;
	}

	/* Align blocks to erase group size to avoid erasing other partitions */
	grp_size = mmc->erase_grp_size;
	blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
	if (info.size >= grp_size)
		blks_size = (info.size - (blks_start - info.start)) &
				(~(grp_size - 1));
	else
		blks_size = 0;

	printf("Erasing blocks " LBAFU " to " LBAFU " due to alignment\n",
	       blks_start, blks_start + blks_size);

	blks = fb_mmc_blk_write(dev_desc, blks_start, blks_size, NULL);

	if (blks != blks_size) {
		pr_err("failed erasing from device %d\n", dev_desc->devnum);
		fastboot_fail("failed erasing from device", response);
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmd);
	fastboot_okay(NULL, response);
}

/**
 * fastboot_mmc_flash_read() - Read image from eMMC to upload buffer
 *
 * @cmd: Named partition to write image to
 * @upload_buffer: buffer to load image data
 * @buffer_size: size of upload_buffer
 * @offset: offset that bytes already loaded
 * @response: Pointer to fastboot response buffer
 *
 * On success, the number of bytes read is returned.
 * On error, -1 is returned.
 */
int64_t fastboot_mmc_flash_read(char *cmd, void *upload_buffer,
			u64 buffer_size, s64 offset, char *response)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	char *cur, *next;
	int64_t start_addr = -1;
	uint64_t length = 0;
	int64_t partition_size = 0;
	int64_t remaining_size = 0;
	int64_t read_size = 0;
	int64_t r = -1;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		fastboot_fail("invalid mmc device", response);
		return -1;
	}

	next = cmd;
	cur = strsep(&next, "@");

	/* addr@length or addr@end_part case */
	if (cur && next && !strict_strtoul(cur, 16, (unsigned long *)&start_addr)) {
		/* addr@length */
		if (strict_strtoul(next, 16, (unsigned long *)&length) < 0) {
			/* addr@end_part_name */
			if (part_get_info_by_name_or_alias(dev_desc, next, &info) < 0) {
				pr_err("cannot find partition: '%s'\n", next);
				fastboot_fail("cannot find partition", response);

				return -1;
			} else {
				length = info.start + info.size;
			}
		}
	} else if (part_get_info_by_name_or_alias(dev_desc, cmd, &info) < 0) {
		pr_err("cannot find partition: '%s'\n", cmd);
		fastboot_fail("cannot find partition", response);

		return -1;
	}

	if (start_addr != -1 && length > 0)
		partition_size = length * dev_desc->blksz;
	else
		partition_size = info.size * info.blksz;

	remaining_size = partition_size - offset;
	if (remaining_size <= 0) {
		pr_err("Error: No remaining bytes to be fetched. partition_size(%llu), offset(%llu)\n",
				partition_size, offset);

		fastboot_fail("Error: No remaining bytes to be fetched.", response);
		return -1;
	}

	printf("remaining(%llu bytes) to be fetched. continue...\n", remaining_size);

	read_size = buffer_size > remaining_size ? remaining_size : buffer_size;

	if (start_addr < 0 && start_addr != -1) {
		pr_err("Error: start_addr: %08lld(%08llx) invalid...\n", start_addr, start_addr);
		fastboot_fail("Error: start_addr(< 0) invalid...\n", response);

		return -1;
	}

	if (start_addr == -1)
		r = read_raw_image(dev_desc, &info, cmd, upload_buffer,
				read_size, offset, response);
	else
		r = read_raw_image_from_addr(dev_desc, (u64)start_addr,
				dev_desc->blksz, upload_buffer,
				read_size, offset, response);

	return r;
}
