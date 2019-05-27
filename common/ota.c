// SPDX-License-Identifier: GPL-2.0+
/*
 * (c) Copyright 2016 by VRT Technology
 *
 * Author:
 *  Stuart Longland <stuartl@vrt.com.au>
 *
 * Based on FAT environment driver
 * (c) Copyright 2011 by Tigris Elektronik GmbH
 *
 * Author:
 *  Maximilian Schwerin <mvs@tigris.de>
 *
 * and EXT4 filesystem implementation
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 */

#include <common.h>

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <memalign.h>
#include <search.h>
#include <errno.h>
#include <ext4fs.h>
#include <mmc.h>
#include <veeprom.h>
#include <ota.h>

static int curr_device = 0;

char *printf_efiname(gpt_entry *pte)
{
	static char name[PARTNAME_SZ + 1];
	int i;

	for (i = 0; i < PARTNAME_SZ; i++) {
		u8 c;
		c = pte->partition_name[i] & 0xff;
		name[i] = c;
	}
	name[PARTNAME_SZ] = 0;

	return name;
}

int get_partition_id(char *partname)
{
	struct blk_desc *mmc_dev;
	struct mmc *mmc;
	struct part_driver *drv;
	int i = 0;
	gpt_entry *gpt_pte = NULL;
	char *name;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	mmc_dev = blk_get_devnum_by_type(IF_TYPE_MMC, curr_device);
	if (mmc_dev !=NULL && mmc_dev->type != DEV_TYPE_UNKNOWN) {
		drv = part_driver_lookup_type(mmc_dev);
		if (!drv)
			return CMD_RET_SUCCESS;

		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, mmc_dev->blksz);

		/* This function validates AND fills in the GPT header and PTE */
		if (is_gpt_valid(mmc_dev, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
			printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
			if (is_gpt_valid(mmc_dev, (mmc_dev->lba - 1),
				gpt_head, &gpt_pte) != 1) {
				printf("%s: *** ERROR: Invalid Backup GPT ***\n",
					__func__);
				return CMD_RET_SUCCESS;
			} else {
				printf("%s: ***        Using Backup GPT ***\n",
					__func__);
			}
		}

	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		/* Stop at the first non valid PTE */
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		name = printf_efiname(&gpt_pte[i]);
		if ( strcmp(name, partname) == 0 ) {
			return i+1;
		}
	}

	/* Remember to free pte */
	free(gpt_pte);
		return 0;
	}

	return CMD_RET_FAILURE;
}

int get_patition_start_lba(char *partname)
{
	struct blk_desc *mmc_dev;
	struct mmc *mmc;
	struct part_driver *drv;
	int i = 0;
	gpt_entry *gpt_pte = NULL;
	char *name;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	mmc_dev = blk_get_devnum_by_type(IF_TYPE_MMC, curr_device);
	if (mmc_dev !=NULL && mmc_dev->type != DEV_TYPE_UNKNOWN) {
		drv = part_driver_lookup_type(mmc_dev);
		if (!drv)
			return CMD_RET_SUCCESS;

		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, mmc_dev->blksz);

		/* This function validates AND fills in the GPT header and PTE */
		if (is_gpt_valid(mmc_dev, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
			printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
			if (is_gpt_valid(mmc_dev, (mmc_dev->lba - 1),
				gpt_head, &gpt_pte) != 1) {
				printf("%s: *** ERROR: Invalid Backup GPT ***\n",
					__func__);
				return CMD_RET_SUCCESS;
			} else {
				printf("%s: ***        Using Backup GPT ***\n",
					__func__);
			}
		}

	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		/* Stop at the first non valid PTE */
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		name = printf_efiname(&gpt_pte[i]);
		if ( strcmp(name, partname) == 0 ) {
			return gpt_pte[i].starting_lba;
		}
	}

	/* Remember to free pte */
	free(gpt_pte);
		return 0;
	}

	return CMD_RET_FAILURE;
}

int get_patition_end_lba(char *partname)
{
	struct blk_desc *mmc_dev;
	struct mmc *mmc;
	struct part_driver *drv;
	int i = 0;
	gpt_entry *gpt_pte = NULL;
	char *name;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	mmc_dev = blk_get_devnum_by_type(IF_TYPE_MMC, curr_device);
	if (mmc_dev !=NULL && mmc_dev->type != DEV_TYPE_UNKNOWN) {
		drv = part_driver_lookup_type(mmc_dev);
		if (!drv)
			return CMD_RET_SUCCESS;

		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, mmc_dev->blksz);

		/* This function validates AND fills in the GPT header and PTE */
		if (is_gpt_valid(mmc_dev, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
			printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
			if (is_gpt_valid(mmc_dev, (mmc_dev->lba - 1),
				gpt_head, &gpt_pte) != 1) {
				printf("%s: *** ERROR: Invalid Backup GPT ***\n",
					__func__);
				return CMD_RET_SUCCESS;
			} else {
				printf("%s: ***        Using Backup GPT ***\n",
					__func__);
			}
		}

	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		/* Stop at the first non valid PTE */
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		name = printf_efiname(&gpt_pte[i]);
		if ( strcmp(name, partname) == 0 ) {
			return gpt_pte[i].ending_lba;
		}
	}

	/* Remember to free pte */
	free(gpt_pte);
		return 0;
	}

	return CMD_RET_FAILURE;
}

unsigned int hex_to_char(unsigned int temp)
{
    uint8_t dst;
    if (temp < 10) {
        dst = temp + '0';
    } else {
        dst = temp -10 +'A';
    }
    return dst;
}

void uint32_to_char(unsigned int temp, char *s)
{
	int i = 0;
	unsigned char dst;
	unsigned char str[10] = { 0 };

	s[0] = '0';
	s[1] = 'x';

	for (i = 0; i < 4; i++) {
		dst = (temp >> (8 * (3 - i))) & 0xff;

		str[2*i + 2] = (dst >> 4) & 0xf;
		str[2*i + 3] = dst & 0xf;
	}

	for (i =2; i < 10; i++) {
		s[i] = hex_to_char(str[i]);
	}

	s[i + 2] = '\0';
}

int ota_update_image(char *name, char *addr, unsigned int bytes)
{
	unsigned int start_lba, end_lba;
	char command[256] = "mmc write ";
	char lba_size[64] = { 0 };
	char partiton_size[64] = { 0 };
	int ret;
	unsigned int part_size;

	start_lba = (unsigned int)get_patition_start_lba(name);
	end_lba = (unsigned int)get_patition_end_lba(name);
	part_size = end_lba - start_lba + 1;
	if (start_lba == CMD_RET_FAILURE) {
		return CMD_RET_FAILURE;
	}

	if (bytes > part_size) {
		printf("Error: image more than partiton size %02x \n", part_size * 512);
		return CMD_RET_FAILURE;
	}

	uint32_to_char(bytes, partiton_size);
	uint32_to_char(start_lba, lba_size);
	strcat(command, addr);
	strcat(command, " ");
	strcat(command, lba_size);
	strcat(command, " ");
	strcat(command, partiton_size);

	printf("command : %s \n", command);
	ret = run_command_list(command, -1, 0);

	if (ret == 0)
		printf("ota update image success!\n");

	return ret;
}


int ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	char *partition_name;
	unsigned int bytes;
	int ret;
	char *ep;

	if (argc != 4) {
		printf("error: parameter number %d not support! \n", argc);
		return CMD_RET_USAGE;
	}

	partition_name = argv[1];

	bytes = simple_strtoul(argv[3], &ep, 16);
	if (ep == argv[3] || *ep != '\0')
			return CMD_RET_USAGE;

	bytes = (bytes + 511)/512;

	/* update image */
	ret = ota_update_image(partition_name, argv[2], bytes);
	if (ret == CMD_RET_FAILURE)
		printf("Error: ota update image faild!\n");

	return ret;
}
