/*
 * (C) Copyright 2018, Linaro Limited
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <avb_verify.h>
#include <blk.h>
#include <fastboot.h>
#include <image.h>
#include <malloc.h>
#include <part.h>
#include <mtd.h>
#include "../cmd/legacy-mtd-utils.h"

#ifdef CONFIG_CMD_SF
extern struct spi_flash *flash;
#endif

static unsigned char avb_root_pub[520] = {
	0x00, 0x00, 0x08, 0x00, 0x15, 0xfa, 0xd8, 0x07,
	0xe8, 0xeb, 0x78, 0x4d, 0x2f, 0x4d, 0x54, 0x91,
	0x7a, 0x7b, 0xb3, 0x3b, 0xdb, 0xe7, 0x69, 0x67,
	0xe4, 0xd1, 0xe4, 0x33, 0x61, 0xa6, 0xf4, 0x82,
	0xaa, 0x62, 0xeb, 0x10, 0x33, 0x8b, 0xa7, 0x66,
	0x0f, 0xeb, 0xa0, 0xa0, 0x42, 0x89, 0x99, 0xb3,
	0xe2, 0xb8, 0x4e, 0x43, 0xc1, 0xfd, 0xb5, 0x8a,
	0xc6, 0x7d, 0xba, 0x15, 0x14, 0xbb, 0x47, 0x50,
	0x33, 0x8e, 0x9d, 0x2b, 0x8a, 0x1c, 0x2b, 0x13,
	0x11, 0xad, 0xc9, 0xe6, 0x1b, 0x1c, 0x9d, 0x16,
	0x7e, 0xa8, 0x7e, 0xcd, 0xce, 0x0c, 0x93, 0x17,
	0x3a, 0x4b, 0xf6, 0x80, 0xa5, 0xcb, 0xfc, 0x57,
	0x5b, 0x10, 0xf7, 0x43, 0x6f, 0x1c, 0xdd, 0xbb,
	0xcc, 0xf7, 0xca, 0x4f, 0x96, 0xeb, 0xbb, 0x9d,
	0x33, 0xf7, 0xd6, 0xed, 0x66, 0xda, 0x43, 0x70,
	0xce, 0xd2, 0x49, 0xee, 0xfa, 0x2c, 0xca, 0x6a,
	0x4f, 0xf7, 0x4f, 0x8d, 0x5c, 0xe6, 0xea, 0x17,
	0x99, 0x0f, 0x35, 0x50, 0xdb, 0x40, 0xcd, 0x11,
	0xb3, 0x19, 0xc8, 0x4d, 0x55, 0x73, 0x26, 0x5a,
	0xe4, 0xc6, 0x3a, 0x48, 0x3a, 0x53, 0xed, 0x08,
	0xd9, 0x37, 0x7b, 0x2b, 0xcc, 0xaf, 0x50, 0xc5,
	0xa1, 0x01, 0x63, 0xcf, 0xa4, 0xa2, 0xed, 0x54,
	0x7f, 0x6b, 0x00, 0xbe, 0x53, 0xce, 0x36, 0x0d,
	0x47, 0xdd, 0xa2, 0xcd, 0xd2, 0x9c, 0xcf, 0x70,
	0x23, 0x46, 0xc2, 0x37, 0x09, 0x38, 0xed, 0xa6,
	0x25, 0x40, 0x04, 0x67, 0x97, 0xd1, 0x37, 0x23,
	0x45, 0x2b, 0x99, 0x07, 0xb2, 0xbd, 0x10, 0xae,
	0x7a, 0x1d, 0x5f, 0x8e, 0x14, 0xd4, 0xba, 0x23,
	0x53, 0x4f, 0x8d, 0xd0, 0xfb, 0x14, 0x84, 0xa1,
	0xc8, 0x69, 0x6a, 0xa9, 0x97, 0x54, 0x3a, 0x40,
	0x14, 0x65, 0x86, 0xa7, 0x6e, 0x98, 0x1e, 0x4f,
	0x93, 0x7b, 0x40, 0xbe, 0xae, 0xba, 0xa7, 0x06,
	0xa6, 0x84, 0xce, 0x91, 0xa9, 0x6e, 0xea, 0x49,
	0x44, 0x49, 0x76, 0xef, 0x06, 0xaa, 0x52, 0x11,
	0x56, 0xd2, 0x58, 0xb7, 0x46, 0x04, 0x42, 0x04,
	0x66, 0x86, 0xa0, 0xea, 0x8b, 0xb1, 0xc2, 0x41,
	0xe2, 0xa2, 0xfb, 0xbb, 0xb9, 0xb8, 0x24, 0x07,
	0x83, 0xdb, 0xdd, 0x7b, 0x0a, 0x7e, 0xcc, 0x40,
	0x1a, 0xeb, 0x9e, 0xe0, 0xea, 0xc3, 0xd1, 0xce,
	0x58, 0xf0, 0x39, 0xbc, 0x78, 0xc0, 0xfd, 0x6c,
	0xfe, 0xf4, 0xfa, 0xfc, 0xcd, 0x53, 0x77, 0xfc,
	0xe1, 0xaf, 0x05, 0xfe, 0x62, 0xed, 0x31, 0xad,
	0xa1, 0xd5, 0x3a, 0xef, 0xbf, 0x31, 0x17, 0x4a,
	0xe4, 0xae, 0xf3, 0x9a, 0xa7, 0xf8, 0x53, 0x6b,
	0x20, 0xc7, 0xcb, 0x07, 0xd3, 0x18, 0x60, 0x71,
	0xf4, 0x7f, 0x2e, 0xc7, 0xc7, 0x76, 0x03, 0x87,
	0x0b, 0x52, 0xf0, 0x12, 0x6d, 0x1f, 0x11, 0x1f,
	0x97, 0xeb, 0x7c, 0x7d, 0xcd, 0xd5, 0x10, 0x49,
	0xd3, 0xc8, 0x37, 0xf2, 0x70, 0x78, 0x16, 0x69,
	0x9a, 0xdc, 0x7b, 0xa6, 0x31, 0xff, 0x40, 0xec,
	0xa8, 0xd6, 0xb4, 0xd9, 0xc6, 0x33, 0x37, 0xd9,
	0x01, 0x0d, 0xc1, 0xe1, 0x97, 0x95, 0x69, 0xac,
	0x1d, 0x88, 0x7a, 0xbb, 0xef, 0x96, 0x88, 0x6c,
	0x01, 0xc2, 0x6c, 0x82, 0x1c, 0xb2, 0x61, 0x4e,
	0xfe, 0x66, 0x96, 0x17, 0x08, 0xf3, 0xfb, 0xdd,
	0x26, 0xf6, 0xac, 0xb6, 0xd3, 0x33, 0x78, 0xd0,
	0x49, 0xf8, 0x54, 0x74, 0x36, 0xd5, 0x20, 0x5f,
	0x6c, 0xb2, 0xe5, 0x90, 0xfd, 0x0e, 0xd9, 0xed,
	0x86, 0x4f, 0x81, 0x58, 0x43, 0xdb, 0xe1, 0x2f,
	0xff, 0xb3, 0x8a, 0xa7, 0x74, 0x1d, 0xf0, 0x70,
	0xbd, 0xc6, 0x7c, 0xac, 0x72, 0x98, 0xc3, 0x15,
	0x25, 0x90, 0xaa, 0x94, 0x4f, 0x22, 0x78, 0xc8,
	0x4f, 0x15, 0x39, 0xe0, 0xfe, 0x50, 0x2d, 0x24,
	0xbe, 0x61, 0xd8, 0xfd, 0x1f, 0x13, 0x0e, 0x26,
	0x07, 0x7e, 0xee, 0x1b, 0x4d, 0xb6, 0xd4, 0xf2,
};


/**
 * ============================================================================
 * Boot states support (GREEN, YELLOW, ORANGE, RED) and dm_verity
 * ============================================================================
 */
char *avb_set_state(AvbOps *ops, enum avb_boot_state boot_state)
{
	struct AvbOpsData *data;
	char *cmdline = NULL;

	if (!ops)
		return NULL;

	data = (struct AvbOpsData *)ops->user_data;
	if (!data)
		return NULL;

	data->boot_state = boot_state;
	switch (boot_state) {
	case AVB_GREEN:
		cmdline = "androidboot.verifiedbootstate=green";
		break;
	case AVB_YELLOW:
		cmdline = "androidboot.verifiedbootstate=yellow";
		break;
	case AVB_ORANGE:
		cmdline = "androidboot.verifiedbootstate=orange";
	case AVB_RED:
		break;
	}

	return cmdline;
}

char *append_cmd_line(char *cmdline_orig, char *cmdline_new)
{
	char *cmd_line;

	if (!cmdline_new)
		return cmdline_orig;

	if (cmdline_orig)
		cmd_line = cmdline_orig;
	else
		cmd_line = " ";

	cmd_line = avb_strdupv(cmd_line, " ", cmdline_new, NULL);

	return cmd_line;
}

static int avb_find_dm_args(char **args, char *str)
{
	int i;

	if (!str)
		return -1;

	for (i = 0; i < AVB_MAX_ARGS && args[i]; ++i) {
		if (strstr(args[i], str))
			return i;
	}

	return -1;
}

static char *avb_set_enforce_option(const char *cmdline, const char *option)
{
	char *cmdarg[AVB_MAX_ARGS];
	char *newargs = NULL;
	int i = 0;
	int total_args;

	memset(cmdarg, 0, sizeof(cmdarg));
	cmdarg[i++] = strtok((char *)cmdline, " ");

	do {
		cmdarg[i] = strtok(NULL, " ");
		if (!cmdarg[i])
			break;

		if (++i >= AVB_MAX_ARGS) {
			printf("%s: Can't handle more then %d args\n",
			       __func__, i);
			return NULL;
		}
	} while (true);

	total_args = i;
	i = avb_find_dm_args(&cmdarg[0], VERITY_TABLE_OPT_LOGGING);
	if (i >= 0) {
		cmdarg[i] = (char *)option;
	} else {
		i = avb_find_dm_args(&cmdarg[0], VERITY_TABLE_OPT_RESTART);
		if (i < 0) {
			printf("%s: No verity options found\n", __func__);
			return NULL;
		}

		cmdarg[i] = (char *)option;
	}

	for (i = 0; i <= total_args; i++)
		newargs = append_cmd_line(newargs, cmdarg[i]);

	return newargs;
}

char *avb_set_ignore_corruption(const char *cmdline)
{
	char *newargs = NULL;

	newargs = avb_set_enforce_option(cmdline, VERITY_TABLE_OPT_LOGGING);
	if (newargs)
		newargs = append_cmd_line(newargs,
					  "androidboot.veritymode=eio");

	return newargs;
}

char *avb_set_enforce_verity(const char *cmdline)
{
	char *newargs;

	newargs = avb_set_enforce_option(cmdline, VERITY_TABLE_OPT_RESTART);
	if (newargs)
		newargs = append_cmd_line(newargs,
					  "androidboot.veritymode=enforcing");
	return newargs;
}

#if defined CONFIG_HB_BOOT_FROM_NOR
/**
 * ============================================================================
 * IO(spi nor) auxiliary functions
 * ============================================================================
 */
static uint64_t sf_read_and_flush(struct spi_flash *flash,
					struct mtd_info *part,
					lbaint_t start,
					lbaint_t len,
					void *buffer)
{
	uint64_t blks;
	if ((start + len) > (part->offset + part->size)) {
		len = part->offset + part->size - start;
		debug("%s: len aligned to nor partition bounds (%ld)\n",
		       __func__, len);
	}
	blks = spi_flash_read(flash, start, len, buffer);
	if (!blks) {
		blks = len;
	} else {
		avb_error("Error: read nor flash fail\n");
		return 0;
	}
	/* flush cache after read */
	flush_cache((ulong)buffer, len);

	return blks;
}

static uint64_t sf_write(struct spi_flash *flash, struct mtd_info *part,
					lbaint_t start, lbaint_t len, void *buffer)
{
	int ret = 0, bytes_written = 0, tmp_len = 0;
	u8 *tmp_buf;
	tmp_buf = (u8 *) malloc(sizeof(u8) * len);
	if (tmp_buf == NULL) {
		printf("avb sf malloc %lu Bytes failed!\n", sizeof(u8) * len);
		return -1;
	}
	if ((start + len) > (part->offset + part->size)) {
		len = part->offset + part->size - start;
		debug("%s: len aligned to nor partition bounds (%ld)\n",
		       __func__, len);
	}
	while(len) {
		if (len > flash->sector_size) {
			tmp_len = flash->sector_size;
		} else {
			ret = spi_flash_read(flash, start, tmp_len, tmp_buf);
			memcpy(tmp_buf, buffer, tmp_len);
		}
		ret = spi_flash_erase(flash, start, tmp_len);
		if (ret) {
			avb_error("SPI Flash Erase failed!\n");
			return -1;
		}
		bytes_written += spi_flash_write(flash, start, tmp_len, tmp_buf);
		len -= tmp_len;
		start += tmp_len;
		buffer += tmp_len;
	}

	return bytes_written;
}

static struct mtd_info *sf_get_partition(const char *partition)
{
	int dev_nb = 0;
	struct mtd_info *mtd = __mtd_next_device(0);;

	/* Ensure all devices (and their partitions) are probed */
	if (!mtd || list_empty(&(mtd->partitions))) {
		mtd_probe_devices();
		mtd_for_each_device(mtd) {
			dev_nb++;
		}
		if (!dev_nb) {
			run_command("sf probe", 0);
			mtd_probe_devices();
			mtd_for_each_device(mtd) {
				dev_nb++;
			}
			if (!dev_nb) {
				avb_error("No MTD device found, abort\n");
				return NULL;
			}
		}
		mtd = __mtd_next_device(0);
	}

	return get_mtd_device_nm(partition);
}

static AvbIOResult sf_byte_io(AvbOps *ops,
			       const char *partition,
			       s64 offset,
			       size_t num_bytes,
			       void *buffer,
			       size_t *out_num_read,
			       enum io_type io_type)
{
	ulong ret;
	u64 start_offset;
	size_t io_cnt = 0;
	struct mtd_info *part_ptr;

	if (!partition || !buffer || io_type > IO_WRITE)
		return AVB_IO_RESULT_ERROR_IO;

	part_ptr = sf_get_partition(partition);
	offset = offset < 0 ? part_ptr->size + offset : offset;
	start_offset = part_ptr->offset + offset;
	if (part_ptr == NULL)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	if (io_type == IO_READ) {
		ret = sf_read_and_flush(flash, part_ptr,
						start_offset,
						num_bytes, buffer);
	} else {
		ret = sf_write(flash, part_ptr,
				start_offset,
				num_bytes, buffer);
	}

	io_cnt += ret;

	/* Set counter for read operation */
	if (io_type == IO_READ && out_num_read)
		*out_num_read = io_cnt;

	return AVB_IO_RESULT_OK;
}

#elif defined CONFIG_HB_BOOT_FROM_NAND
/**
 * ============================================================================
 * IO(spi nand) auxiliary functions
 * ============================================================================
 */
static uint64_t nand_read_and_flush(struct ubi_volume *part,
					lbaint_t start,
					lbaint_t len,
					void *buffer)
{
	lbaint_t total_len = len;
	uint64_t ret = 0;

	ret = ubi_volume_read(part->name, buffer, 0);
	if (ret) {
		printf("UBI read %s failed!", part->name);
		return ret;
	}
	if ((start + len) > part->used_bytes) {
		debug("Attempting to read beyond boundary, stop at boundary\n");
		len = part->used_bytes - start;
	}
	memmove(buffer, buffer + start, len);
	return total_len;
}

static uint64_t nand_write(struct ubi_volume *part, lbaint_t start,
			       lbaint_t len, void *buffer)
{
	uint64_t ret = 0, vol_size = 0;
	void *tmp_buf;

	tmp_buf = (void *) malloc(part->used_bytes * sizeof(char));
	if (tmp_buf == NULL) {
		printf("avb nand malloc %llu Bytes failed!\n",
				part->used_bytes * sizeof(char));
		return -1;
	}
	ret = ubi_volume_read(part->name, tmp_buf, 0);
	if (ret) {
		printf("UBI Volume %s access failed!\n", part->name);
		return -1;
	}
	vol_size = part->reserved_pebs * (part->ubi->leb_size - part->data_pad);
	if ((start + len) > vol_size) {
		debug("Attempting to write outside of boundary, stops at boundary.\n");
		len = vol_size - start;
	}

	memcpy(tmp_buf + start, buffer, len);
	ret = ubi_volume_write(part->name, tmp_buf, len);

	return ret;
}

static struct ubi_volume *nand_get_partition(const char *partition)
{
	if (ubi_part("boot", NULL))
		return NULL;
	return ubi_find_volume(partition);
}

static AvbIOResult nand_byte_io(AvbOps *ops,
			       const char *partition,
			       s64 offset,
			       size_t num_bytes,
			       void *buffer,
			       size_t *out_num_read,
			       enum io_type io_type)
{
	ulong ret;
	u64 start_offset;
	size_t io_cnt = 0;
	struct ubi_volume *part = NULL;

	if (!partition || !buffer || io_type > IO_WRITE)
		return AVB_IO_RESULT_ERROR_IO;

	part = nand_get_partition(partition);
	if (part == NULL)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	start_offset = offset < 0 ? part->used_bytes + offset : offset;
	if (io_type == IO_READ) {
		ret = nand_read_and_flush(part,
						start_offset,
						num_bytes, buffer);
	} else {
		ret = nand_write(part,
						start_offset,
						num_bytes,  buffer);
	}

	io_cnt += ret;

	/* Set counter for read operation */
	if (io_type == IO_READ && out_num_read)
		*out_num_read = io_cnt;

	return AVB_IO_RESULT_OK;
}

#elif CONFIG_HB_BOOT_FROM_MMC
/**
 * ============================================================================
 * IO(mmc) auxiliary functions
 * ============================================================================
 */
static uint64_t mmc_read_and_flush(struct mmc_part *part,
					lbaint_t start,
					lbaint_t sectors,
					void *buffer)
{
	uint64_t blks;
	void *tmp_buf;
	size_t buf_size;
	bool unaligned = is_buf_unaligned(buffer);

	if (start < part->info.start) {
		debug("%s: partition start out of bounds\n", __func__);
		return 0;
	}
	if ((start + sectors) > (part->info.start + part->info.size)) {
		sectors = part->info.start + part->info.size - start;
		debug("%s: read sector aligned to partition bounds (%ld)\n",
		       __func__, sectors);
	}

	/*
	 * Reading fails on unaligned buffers, so we have to
	 * use aligned temporary buffer and then copy to destination
	 */

	if (unaligned) {
		avb_debug("Handling unaligned read buffer..\n");
		tmp_buf = get_sector_buf();
		buf_size = get_sector_buf_size();
		if (sectors > buf_size / part->info.blksz)
			sectors = buf_size / part->info.blksz;
	} else {
		tmp_buf = buffer;
	}

	blks = blk_dread(part->mmc_blk,
			 start, sectors, tmp_buf);
	/* flush cache after read */
	flush_cache((ulong)tmp_buf, sectors * part->info.blksz);

	if (unaligned)
		memcpy(buffer, tmp_buf, sectors * part->info.blksz);

	return blks;
}

static uint64_t mmc_write(struct mmc_part *part, lbaint_t start,
			       lbaint_t sectors, void *buffer)
{
	void *tmp_buf;
	size_t buf_size;
	bool unaligned = is_buf_unaligned(buffer);

	if (start < part->info.start) {
		debug("%s: partition start out of bounds\n", __func__);
		return 0;
	}
	if ((start + sectors) > (part->info.start + part->info.size)) {
		sectors = part->info.start + part->info.size - start;
		debug("%s: sector aligned to partition bounds (%ld)\n",
		       __func__, sectors);
	}
	if (unaligned) {
		tmp_buf = get_sector_buf();
		buf_size = get_sector_buf_size();
		avb_debug("Handling unaligned wrire buffer..\n");
		if (sectors > buf_size / part->info.blksz)
			sectors = buf_size / part->info.blksz;

		memcpy(tmp_buf, buffer, sectors * part->info.blksz);
	} else {
		tmp_buf = buffer;
	}

	return blk_dwrite(part->mmc_blk,
			  start, sectors, tmp_buf);
}

static struct mmc_part *get_partition(AvbOps *ops, const char *partition)
{
	int ret;
	u8 dev_num;
	int part_num = 0;
	struct mmc_part *part;
	struct blk_desc *mmc_blk;
	struct AvbOpsData *ops_data = container_of(ops, struct AvbOpsData, ops);

	part = malloc(sizeof(struct mmc_part));
	if (!part)
		return NULL;

	dev_num = get_boot_device(ops);

	if (ops_data->if_type == IF_TYPE_MMC) {
		part->mmc = find_mmc_device(dev_num);
		if (!part->mmc) {
			printf("No MMC device at slot %x\n", dev_num);
			goto err;
		}

		if (mmc_init(part->mmc)) {
			avb_error("MMC initialization failed\n");
			goto err;
		}

		ret = mmc_switch_part(part->mmc, part_num);
		if (ret)
			goto err;

		mmc_blk = mmc_get_blk_desc(part->mmc);
	} else {
		mmc_blk = blk_get_devnum_by_type(ops_data->if_type, dev_num);
	}

	if (!mmc_blk) {
		avb_error("Error - failed to obtain block descriptor\n");
		goto err;
	}

	ret = part_get_info_by_name(mmc_blk, partition, &part->info);
	if (!ret) {
		printf("Can't find partition '%s'\n", partition);
		goto err;
	}

	part->dev_num = dev_num;
	part->mmc_blk = mmc_blk;

	return part;
err:
	free(part);
	return NULL;
}

static AvbIOResult mmc_byte_io(AvbOps *ops,
			       const char *partition,
			       s64 offset,
			       size_t num_bytes,
			       void *buffer,
			       size_t *out_num_read,
			       enum io_type io_type)
{
	ulong ret;
	struct mmc_part *part;
	u64 start_offset, start_sector, sectors, residue;
	u8 *tmp_buf;
	size_t io_cnt = 0;

	if (!partition || !buffer || io_type > IO_WRITE)
		return AVB_IO_RESULT_ERROR_IO;

	part = get_partition(ops, partition);
	if (!part)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	if (!part->info.blksz)
		return AVB_IO_RESULT_ERROR_IO;

	start_offset = calc_offset(part, offset);
	while (num_bytes) {
		start_sector = start_offset / part->info.blksz;
		sectors = num_bytes / part->info.blksz;
		/* handle non block-aligned reads */
		if (start_offset % part->info.blksz ||
		    num_bytes < part->info.blksz) {
			tmp_buf = get_sector_buf();
			if (start_offset % part->info.blksz) {
				residue = part->info.blksz -
					(start_offset % part->info.blksz);
				if (residue > num_bytes)
					residue = num_bytes;
			} else {
				residue = num_bytes;
			}

			if (io_type == IO_READ) {
				ret = mmc_read_and_flush(part,
							 part->info.start +
							 start_sector,
							 1, tmp_buf);

				if (ret != 1) {
					printf("%s: read error (%ld, %lld)\n",
					       __func__, ret, start_sector);
					return AVB_IO_RESULT_ERROR_IO;
				}
				/*
				 * if this is not aligned at sector start,
				 * we have to adjust the tmp buffer
				 */
				tmp_buf += (start_offset % part->info.blksz);
				memcpy(buffer, (void *)tmp_buf, residue);
			} else {
				ret = mmc_read_and_flush(part,
							 part->info.start +
							 start_sector,
							 1, tmp_buf);

				if (ret != 1) {
					printf("%s: read error (%ld, %lld)\n",
					       __func__, ret, start_sector);
					return AVB_IO_RESULT_ERROR_IO;
				}
				memcpy((void *)tmp_buf +
					start_offset % part->info.blksz,
					buffer, residue);

				ret = mmc_write(part, part->info.start +
						start_sector, 1, tmp_buf);
				if (ret != 1) {
					printf("%s: write error (%ld, %lld)\n",
					       __func__, ret, start_sector);
					return AVB_IO_RESULT_ERROR_IO;
				}
			}

			io_cnt += residue;
			buffer += residue;
			start_offset += residue;
			num_bytes -= residue;
			continue;
		}

		if (sectors) {
			if (io_type == IO_READ) {
				ret = mmc_read_and_flush(part,
							 part->info.start +
							 start_sector,
							 sectors, buffer);
			} else {
				ret = mmc_write(part,
						part->info.start +
						start_sector,
						sectors, buffer);
			}

			if (!ret) {
				printf("%s: sector read error\n", __func__);
				return AVB_IO_RESULT_ERROR_IO;
			}

			io_cnt += ret * part->info.blksz;
			buffer += ret * part->info.blksz;
			start_offset += ret * part->info.blksz;
			num_bytes -= ret * part->info.blksz;
		}
	}

	/* Set counter for read operation */
	if (io_type == IO_READ && out_num_read)
		*out_num_read = io_cnt;

	return AVB_IO_RESULT_OK;
}
#endif

/**
 * ============================================================================
 * AVB 2.0 operations
 * ============================================================================
 */

/**
 * read_from_partition() - reads @num_bytes from  @offset from partition
 * identified by a string name
 *
 * @ops: contains AVB ops handlers
 * @partition_name: partition name, NUL-terminated UTF-8 string
 * @offset: offset from the beginning of partition
 * @num_bytes: amount of bytes to read
 * @buffer: destination buffer to store data
 * @out_num_read:
 *
 * @return:
 *      AVB_IO_RESULT_OK, if partition was found and read operation succeed
 *      AVB_IO_RESULT_ERROR_IO, if i/o error occurred from the underlying i/o
 *            subsystem
 *      AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION, if there is no partition with
 *      the given name
 */
static AvbIOResult read_from_partition(AvbOps *ops,
				       const char *partition_name,
				       s64 offset_from_partition,
				       size_t num_bytes,
				       void *buffer,
				       size_t *out_num_read)
{
#if defined CONFIG_HB_BOOT_FROM_NOR
	return sf_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, buffer, out_num_read, IO_READ);
#elif defined CONFIG_HB_BOOT_FROM_NAND
	return nand_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, buffer, out_num_read, IO_READ);
#elif defined CONFIG_HB_BOOT_FROM_MMC
	return mmc_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, buffer, out_num_read, IO_READ);
#endif
}

/**
 * write_to_partition() - writes N bytes to a partition identified by a string
 * name
 *
 * @ops: AvbOps, contains AVB ops handlers
 * @partition_name: partition name
 * @offset_from_partition: offset from the beginning of partition
 * @num_bytes: amount of bytes to write
 * @buf: data to write
 * @out_num_read:
 *
 * @return:
 *      AVB_IO_RESULT_OK, if partition was found and read operation succeed
 *      AVB_IO_RESULT_ERROR_IO, if input/output error occurred
 *      AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION, if partition, specified in
 *            @partition_name was not found
 */
static AvbIOResult write_to_partition(AvbOps *ops,
				      const char *partition_name,
				      s64 offset_from_partition,
				      size_t num_bytes,
				      const void *buffer)
{
#if defined CONFIG_HB_BOOT_FROM_NOR
	return sf_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, (void *)buffer, NULL, IO_WRITE);
#elif defined CONFIG_HB_BOOT_FROM_NAND
	return nand_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, (void *)buffer, NULL, IO_WRITE);
#elif defined CONFIG_HB_BOOT_FROM_MMC
	return mmc_byte_io(ops, partition_name, offset_from_partition,
			   num_bytes, (void *)buffer, NULL, IO_WRITE);
#endif
}

/**
 * validate_vmbeta_public_key() - checks if the given public key used to sign
 * the vbmeta partition is trusted
 *
 * @ops: AvbOps, contains AVB ops handlers
 * @public_key_data: public key for verifying vbmeta partition signature
 * @public_key_length: length of public key
 * @public_key_metadata:
 * @public_key_metadata_length:
 * @out_key_is_trusted:
 *
 * @return:
 *      AVB_IO_RESULT_OK, if partition was found and read operation succeed
 */
static AvbIOResult validate_vbmeta_public_key(AvbOps *ops,
					      const u8 *public_key_data,
					      size_t public_key_length,
					      const u8
					      *public_key_metadata,
					      size_t
					      public_key_metadata_length,
					      bool *out_key_is_trusted)
{
	if (!public_key_length || !public_key_data || !out_key_is_trusted)
		return AVB_IO_RESULT_ERROR_IO;

	*out_key_is_trusted = false;
	if (public_key_length != sizeof(avb_root_pub))
		return AVB_IO_RESULT_ERROR_IO;

	if (memcmp(avb_root_pub, public_key_data, public_key_length) == 0)
		*out_key_is_trusted = true;

	return AVB_IO_RESULT_OK;
}

/**
 * read_rollback_index() - gets the rollback index corresponding to the
 * location of given by @out_rollback_index.
 *
 * @ops: contains AvbOps handlers
 * @rollback_index_slot:
 * @out_rollback_index: used to write a retrieved rollback index.
 *
 * @return
 *       AVB_IO_RESULT_OK, if the roolback index was retrieved
 */
static AvbIOResult read_rollback_index(AvbOps *ops,
				       size_t rollback_index_slot,
				       u64 *out_rollback_index)
{
	/* For now we always return 0 as the stored rollback index. */
	debug("%s not supported yet\n", __func__);

	if (out_rollback_index)
		*out_rollback_index = 0;

	return AVB_IO_RESULT_OK;
}

/**
 * write_rollback_index() - sets the rollback index corresponding to the
 * location of given by @out_rollback_index.
 *
 * @ops: contains AvbOps handlers
 * @rollback_index_slot:
 * @rollback_index: rollback index to write.
 *
 * @return
 *       AVB_IO_RESULT_OK, if the roolback index was retrieved
 */
static AvbIOResult write_rollback_index(AvbOps *ops,
					size_t rollback_index_slot,
					u64 rollback_index)
{
	/* For now this is a no-op. */
	debug("%s not supported yet\n", __func__);

	return AVB_IO_RESULT_OK;
}

/**
 * read_is_device_unlocked() - gets whether the device is unlocked
 *
 * @ops: contains AVB ops handlers
 * @out_is_unlocked: device unlock state is stored here, true if unlocked,
 *       false otherwise
 *
 * @return:
 *       AVB_IO_RESULT_OK: state is retrieved successfully
 *       AVB_IO_RESULT_ERROR_IO: an error occurred
 */
static AvbIOResult read_is_device_unlocked(AvbOps *ops, bool *out_is_unlocked)
{
	/* For now we always return that the device is unlocked. */

	debug("%s not supported yet\n", __func__);

	*out_is_unlocked = true;

	return AVB_IO_RESULT_OK;
}

/**
 * get_unique_guid_for_partition() - gets the GUID for a partition identified
 * by a string name
 *
 * @ops: contains AVB ops handlers
 * @partition: partition name (NUL-terminated UTF-8 string)
 * @guid_buf: buf, used to copy in GUID string. Example of value:
 *      527c1c6d-6361-4593-8842-3c78fcd39219
 * @guid_buf_size: @guid_buf buffer size
 *
 * @return:
 *      AVB_IO_RESULT_OK, on success (GUID found)
 *      AVB_IO_RESULT_ERROR_IO, if incorrect buffer size (@guid_buf_size) was
 *             provided
 *      AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION, if partition was not found
 */
static AvbIOResult get_unique_guid_for_partition(AvbOps *ops,
						 const char *partition,
						 char *guid_buf,
						 size_t guid_buf_size)
{
#if defined (CONFIG_HB_BOOT_FROM_NOR) || defined (CONFIG_HB_BOOT_FROM_NAND)
	memset(guid_buf, 0xff, guid_buf_size);
#else
	struct mmc_part *part;
	size_t uuid_size;

	part = get_partition(ops, partition);
	if (!part)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	uuid_size = sizeof(part->info.uuid);
	if (uuid_size > guid_buf_size)
		return AVB_IO_RESULT_ERROR_IO;

	memcpy(guid_buf, part->info.uuid, uuid_size);
	guid_buf[uuid_size - 1] = 0;
#endif

	return AVB_IO_RESULT_OK;
}

/**
 * get_size_of_partition() - gets the size of a partition identified
 * by a string name
 *
 * @ops: contains AVB ops handlers
 * @partition: partition name (NUL-terminated UTF-8 string)
 * @out_size_num_bytes: returns the value of a partition size
 *
 * @return:
 *      AVB_IO_RESULT_OK, on success (GUID found)
 *      AVB_IO_RESULT_ERROR_INSUFFICIENT_SPACE, out_size_num_bytes is NULL
 *      AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION, if partition was not found
 */
static AvbIOResult get_size_of_partition(AvbOps *ops,
					 const char *partition,
					 u64 *out_size_num_bytes)
{
	if (!out_size_num_bytes)
		return AVB_IO_RESULT_ERROR_INSUFFICIENT_SPACE;

#if defined CONFIG_HB_BOOT_FROM_NOR
	struct mtd_info *cur_part = sf_get_partition(partition);
	if (!cur_part)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	*out_size_num_bytes = cur_part->size;
#elif defined CONFIG_HB_BOOT_FROM_NAND
	struct ubi_volume *cur_part = nand_get_partition(partition);
	if (!cur_part)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	*out_size_num_bytes = cur_part->reserved_pebs
						* (cur_part->ubi->leb_size - cur_part->data_pad);
#elif defined CONFIG_HB_BOOT_FROM_MMC
	struct mmc_part *part;

	part = get_partition(ops, partition);
	if (!part)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	*out_size_num_bytes = part->info.blksz * part->info.size;
#endif
	return AVB_IO_RESULT_OK;
}

/**
 * ============================================================================
 * AVB2.0 AvbOps alloc/initialisation/free
 * ============================================================================
 */
AvbOps *avb_ops_alloc(const char *if_typename, int boot_device)
{
	enum if_type if_type = IF_TYPE_UNKNOWN;
	struct AvbOpsData *ops_data;

#if defined CONFIG_HB_BOOT_FROM_NOR
	avb_debug("Hobot SPI-NOR AVB Verify\n");
	if (strcmp(if_typename, "sf")) {
		avb_error("Wrong interface passed in, please use sf!\n");
		return NULL;
	}
	if_type = IF_TYPE_COUNT + 1;
	char *s;
	int ret;
	if (!flash) {
		s = "sf probe";
		ret = run_command_list(s, -1, 0);
		if (ret)
			return NULL;
	}
#elif defined CONFIG_HB_BOOT_FROM_NAND
	avb_debug("Hobot SPI-NAND AVB Verify\n");
	if_type = IF_TYPE_COUNT + 2;
	if (strcmp(if_typename, "nand")) {
		avb_error("Wrong interface passed in, please use nand!\n");
		return NULL;
	}
	if (ubi_part("boot", NULL)) {
		avb_error("UBI Volume Init Failed!\n");
		return NULL;
	}
#elif defined CONFIG_HB_BOOT_FROM_MMC
	for (int i = 0; i < IF_TYPE_COUNT; i++) {
		const char *if_typename_str = blk_get_if_type_name(i);
		if (if_typename_str && !strcmp(if_typename, if_typename_str)) {
			if_type = i;
			break;
		}
	}
#endif

	if (if_type == IF_TYPE_UNKNOWN) {
		printf("%s: Unknow interface type '%s'\n", __func__, if_typename);
		return NULL;
	}

	ops_data = avb_calloc(sizeof(struct AvbOpsData));
	if (!ops_data)
		return NULL;

	ops_data->if_type = if_type;
	ops_data->ops.user_data = ops_data;

	ops_data->ops.read_from_partition = read_from_partition;
	ops_data->ops.write_to_partition = write_to_partition;
	ops_data->ops.validate_vbmeta_public_key = validate_vbmeta_public_key;
	ops_data->ops.read_rollback_index = read_rollback_index;
	ops_data->ops.write_rollback_index = write_rollback_index;
	ops_data->ops.read_is_device_unlocked = read_is_device_unlocked;
	ops_data->ops.get_unique_guid_for_partition =
		get_unique_guid_for_partition;
	ops_data->ops.get_size_of_partition = get_size_of_partition;
	ops_data->dev = boot_device;

	return &ops_data->ops;
}

void avb_ops_free(AvbOps *ops)
{
	struct AvbOpsData *ops_data;

	if (!ops)
		return;

	ops_data = ops->user_data;

	if (ops_data)
		avb_free(ops_data);
}
