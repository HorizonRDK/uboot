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
#include <spi_flash.h>
#include <hb_info.h>

static void bootinfo_update_spl(char * addr, unsigned int spl_size);

static int curr_device = 0;
extern struct spi_flash *flash;
extern bool recovery_sys_enable;

int get_emmc_size(uint64_t *size)
{
	struct mmc *mmc = NULL;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	*size = mmc->capacity;
	return CMD_RET_SUCCESS;
}

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

static unsigned int get_patition_lba(char *partname,
	unsigned int *start_lba, unsigned int *end_lba)
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
				*start_lba = gpt_pte[i].starting_lba;
				*end_lba = gpt_pte[i].ending_lba;
				break;
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

static unsigned int partition_status_flag(char *partition)
{
	unsigned int flag = 0;

	char parts[10][16] = {"spl", "uboot", "kernel", "system", "app",
		"userdata"};

	for (unsigned int i = 0; i < 16; i++) {
		if (strcmp(parts[i], partition) == 0) {
			flag = i;
			break;
		}
	}
	return flag;
}

static int ota_nor_update_image(char *name, char *addr, unsigned int bytes)
{
	unsigned int part_addr;
	unsigned int part_size;
	int ret;
	char *s;
	void *realaddr;
	unsigned int sector = (bytes + NOR_SECTOR_SIZE -1)/NOR_SECTOR_SIZE;

	if (strcmp(name, "uboot") == 0) {
		part_addr = NOR_UBOOT_ADDR;
		part_size = NOR_UBOOT_MAX_SIZE;
	} else if (strcmp(name, "kernel") == 0) {
		part_addr = NOR_KERNEL_ADDR;
		part_size = NOR_KERNEL_MAX_SIZE;
	} else if ((strcmp(name, "system") == 0)) {
		part_addr = NOR_ROOTFS_ADDR;
		part_size = NOR_ROOTFS_MAX_SIZE;
	} else if ((strcmp(name, "app") == 0)) {
		part_addr = NOR_APP_ADDR;
		part_size = NOR_APP_MAX_SIZE;
	} else if ((strcmp(name, "all") == 0)) {
		part_addr = 0;
		part_size = 0x4000000;
	} else {
		printf("error: partition name %s not support! \n", name);
		return CMD_RET_FAILURE;
	}

	if (bytes > part_size) {
		printf("Error: image more than partition size %02x \n", part_size);
		return CMD_RET_FAILURE;
	}

	if (!flash) {
		s = "sf probe";
		ret = run_command_list(s, -1, 0);
		if (ret < 0) {
			printf("flash init failed !\n");
			return CMD_RET_FAILURE;
		}
	}

	printf("sf erase %02x %02x\n", part_addr, sector * NOR_SECTOR_SIZE);
	ret = spi_flash_erase(flash, part_addr, sector * NOR_SECTOR_SIZE);
	if (ret < 0) {
		printf(" sf erase error \n");
		return ret;
	}

	realaddr = (void *)simple_strtoul(addr, NULL, 16);
	printf("sf write %s %02x %02x\n", (char *)realaddr, part_addr,
		sector * NOR_SECTOR_SIZE);
	ret = spi_flash_write(flash, part_addr, bytes, realaddr);
	if (ret < 0) {
		printf(" sf write error \n");
		return ret;
	}

	return ret;
}

static int ota_mmc_update_image(char *name, char *addr, unsigned int bytes)
{
	unsigned int start_lba = 0, end_lba = 0;
	char command[256] = "mmc write ";
	char lba_size[64] = { 0 };
	char *s;
	int ret;
	unsigned int sector = (bytes + 511)/512;
	unsigned int part_size;
	void *realaddr;

	if (strcmp(name, "gpt-main") == 0) {
		printf("in gpt-main\n");
		if (bytes != 34*512) {
			printf("Error: gpt-main size(%x) is not equal to 0x%x\n", bytes, 34*512);
			return CMD_RET_FAILURE;
		}
		sprintf(command, "%s %s 0 %x", command, addr, 34);
	} else if (strcmp(name, "gpt-backup") == 0) {
		printf("in gpt-backup\n");
		if (bytes != 33*512) {
			printf("Error: gpt-backup size(%x) is not equal to 0x%x\n", bytes, 33*512);
			return CMD_RET_FAILURE;
		}
		get_patition_lba("userdata", &start_lba, &end_lba);
		uint32_to_char(end_lba+1, lba_size);
		sprintf(command, "%s %s %s %x", command, addr, lba_size, 33);
	} else if (strcmp(name, "all") == 0){
		printf("in all\n");

		sprintf(command, "%s %s 0 %x", command, addr, sector);
	} else {
		get_patition_lba(name, &start_lba, &end_lba);
		part_size = end_lba - start_lba + 1;
		if (start_lba == end_lba) {
			return CMD_RET_FAILURE;
		}

		if (sector > part_size) {
			printf("Error: image more than partiton size %02x \n", part_size * 512);
			return CMD_RET_FAILURE;
		}

		sprintf(command, "%s %s %x %x", command, addr, start_lba, sector);

		if (strcmp(name, "sbl") == 0) {
			realaddr = (void *)simple_strtoul(addr, NULL, 16);
			bootinfo_update_spl(realaddr, bytes);
		}
	}

	printf("command : %s \n", command);
	ret = run_command_list(command, -1, 0);
	if (ret < 0)
		return ret;

	if (strcmp(name, "all") == 0) {
		s = "gpt extend mmc 0";
		ret = run_command_list(s, -1, 0);
	}

	return ret;
}

int ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	char *partition_name;
	unsigned int bytes;
	int ret;
	char *ep;
	unsigned int boot_mode;

	if ((argc != 4) && (argc != 5)) {
		printf("error: parameter number %d not support! \n", argc);
		return CMD_RET_USAGE;
	}

	if (argc == 5) {
		if (strcmp(argv[4],"emmc") == 0) {
			boot_mode = PIN_2ND_EMMC;
		} else if (strcmp(argv[4],"nor") == 0) {
			boot_mode = PIN_2ND_NOR;
		} else {
			printf("error: parameter %s not support! \n", argv[4]);
			return CMD_RET_USAGE;
		}
	} else {
		boot_mode = hb_boot_mode_get();
	}
	partition_name = argv[1];

	bytes = simple_strtoul(argv[3], &ep, 16);
	if (ep == argv[3] || *ep != '\0')
			return CMD_RET_USAGE;

	/* update image */
	if ((boot_mode == PIN_2ND_NOR) || (boot_mode = PIN_2ND_NAND))
		ret = ota_nor_update_image(partition_name, argv[2], bytes);
	else
		ret = ota_mmc_update_image(partition_name, argv[2], bytes);

	if (ret == 0)
		printf("ota update image success!\n");
	else
		printf("Error: ota update image failed!\n");

	return ret;
}

void ota_update_failed_output(char *boot_reason, char *partition)
{
	printf("*************************************************\n");
	printf("Error: update %s failed! \n", boot_reason);
	printf("	   into %s backup partition! \n", partition);
	printf("*************************************************\n");
}

void ota_get_update_status(char *up_flag, char *partstatus,
	char *boot_reason)
{
	veeprom_read(VEEPROM_UPDATE_FLAG_OFFSET, up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

	veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
}

void ota_update_set_bootdelay(char *boot_reason)
{
	/* update: setting delay 0 */
	if (strcmp(boot_reason, "normal") != 0)
		env_set("bootdelay", "0");
}

int ota_normal_boot(bool boot_flag, char *partition)
{
	int boot_mode = hb_boot_mode_get();

	if ((boot_mode == PIN_2ND_NOR) || (boot_mode = PIN_2ND_NAND)) {
		return 0;
	} else {
		if (boot_flag == 1) {
			strcat(partition, "bak");
		}
		return get_partition_id(partition);
	}
}


bool ota_kernel_or_system_update(char up_flag, bool part_status)
{
	bool flash_success, first_try, app_success;
	bool boot_flag = part_status;

	/* update flag */
	flash_success = (up_flag >> 2) & 0x1;
	first_try = (up_flag >> 1) & 0x1;
	app_success = up_flag & 0x1;

	if (flash_success == 0)
		boot_flag = part_status^1;
	else if (first_try == 1) {
		up_flag = up_flag & 0xd;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);
	} else if(app_success == 0) {
			boot_flag = part_status^1;
	}

	return boot_flag;
}

bool ota_all_update(char up_flag, bool part_status)
{
	bool flash_success, app_success;
	bool boot_flag = part_status;
	char count;

	/* update flag */
	flash_success = (up_flag >> 2) & 0x1;
	app_success = up_flag & 0x1;

	veeprom_read(VEEPROM_COUNT_OFFSET, &count, VEEPROM_COUNT_SIZE);

	if (flash_success == 0)
		boot_flag = part_status^1;
	else if (count > 0 ) {
		count = count - 1;
		veeprom_write(VEEPROM_COUNT_OFFSET, &count,
			VEEPROM_COUNT_SIZE);
	} else if(app_success == 0) {
		boot_flag = part_status^1;
	}

	return boot_flag;
}

void ota_recovery_mode_set(bool upflag)
{
	char *s = NULL, *fdt = NULL;
	char cmd[64] = { 0 };
	char boot_reason[16] = "recovery";
	int ret;
	unsigned int kernel_id;
	int boot_mode = hb_boot_mode_get();

	if (recovery_sys_enable) {
		if (upflag) {
			veeprom_write(VEEPROM_RESET_REASON_OFFSET, boot_reason,
				VEEPROM_RESET_REASON_SIZE);
		}

		if (boot_mode == PIN_2ND_EMMC) {
			/* env bootfile set*/
			s = "recovery.gz\0";
			env_set("bootfile", s);

			/* config env fdtimage */
			kernel_id = get_partition_id("kernel");
			fdt = env_get("fdtimage");
			snprintf(cmd, sizeof(cmd), "ext4load mmc 0:%d ${fdt_addr} "
				"recovery_dtb/%s", kernel_id, fdt);
			ret = run_command_list(cmd, -1, 0);
			if (ret == 0) {
				snprintf(cmd, sizeof(cmd), "recovery_dtb/%s", fdt);
				printf("recovery fdtimage: %s\n", cmd);
				env_set("fdtimage", cmd);
			}
		}
	}
}

void ota_ab_boot_bak_partition(unsigned int *rootfs_id,
	unsigned int *kernel_id)
{
	char partstatus;
	bool boot_flag, part_status;
	char rootfs[32] = "system";
	char kernel[32] = "kernel";

	veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, &partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	/* get kernel backup partition id */
	part_status = (partstatus >> partition_status_flag(rootfs)) & 0x1;
	boot_flag = part_status ^ 1;

	if (boot_flag == 1)
		snprintf(rootfs, sizeof(rootfs), "%sbak", rootfs);

	*rootfs_id = get_partition_id(rootfs);

	/* get system backup partition id */
	part_status = (partstatus >> partition_status_flag(kernel)) & 0x1;
	boot_flag = part_status ^ 1;

	if (boot_flag == 1)
		snprintf(kernel, sizeof(kernel), "%sbak", kernel);
	*kernel_id = get_partition_id(kernel);

	return;
}

unsigned int ota_check_update_success_flag(void)
{
	char boot_reason[64] = { 0 };
	bool update_success;
	char up_flag, partstatus;
	char upmode[16] = { 0 };

	ota_get_update_status(&up_flag, &partstatus, boot_reason);

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	update_success = (up_flag >> 3) & 0x1;
	if ((update_success == 0) && (strcmp(upmode, "golden") == 0)) {
		/* when update uboot failed in golden mode, into recovery system */
		ota_recovery_mode_set(true);
		return 1;
	} else {
		return 0;
	}
}

unsigned int ota_uboot_update_check(char *partition) {
	char boot_reason[64] = { 0 };
	char up_flag, partstatus;
	bool boot_bak, part_status;
	bool boot_flag;
	char upmode[16] = { 0 };
	int boot_mode = hb_boot_mode_get();

	ota_get_update_status(&up_flag, &partstatus, boot_reason);

	part_status = (partstatus >> partition_status_flag(partition)) & 0x1;
	boot_flag = part_status;
	boot_bak = part_status^1;

	/* update: setting delay 0 */
	ota_update_set_bootdelay(boot_reason);

	/* normal boot */
	if ((strcmp(boot_reason, partition) != 0) &&
		(strcmp(boot_reason, "all") != 0))  {
		return ota_normal_boot(boot_flag, partition);
	}

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	/* update kernel , system or all */
	if (strcmp(boot_reason, partition) == 0) {
		boot_flag = ota_kernel_or_system_update(up_flag, part_status);
	} else if(strcmp(boot_reason, "all") == 0) {
		boot_flag = ota_all_update(up_flag, part_status);
	}

	/* If update failed, using backup partition */
	if (boot_flag == boot_bak) {
		veeprom_read(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);
		up_flag = up_flag & 0x7;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

		if (strcmp(upmode, "golden") == 0)
			/* update failed, enter recovery mode */
			ota_recovery_mode_set(true);
		else
			ota_update_failed_output(boot_reason, partition);
	}

	if ((boot_mode == PIN_2ND_NOR) || (boot_mode = PIN_2ND_NAND)) {
		if (boot_flag == boot_bak)
			return 1;
		else
			return 0;
	} else {
		if (boot_flag == 1)
			strcat(partition, "bak");

		return get_partition_id(partition);
	}
}

uint32_t hb_do_cksum(const uint8_t *buff, uint32_t len)
{
	uint32_t result = 0;
	uint32_t i = 0;

	for (i=0; i<len; i++) {
		result += buff[i];
	}

	return result;
}

static void write_bootinfo(void)
{
	int ret;
	char cmd[256] = {0};

	sprintf(cmd, "mmc write 0x%x 0 0x1", HB_BOOTINFO_ADDR);
	ret = run_command_list(cmd, -1, 0);
	debug("cmd:%s, ret:%d\n", cmd, ret);

	if (ret == 0)
		printf("write bootinfo success!\n");
}

static void bootinfo_cs_spl(char * addr, unsigned int size, struct hb_info_hdr * pinfo)
{
	unsigned int csum;

	csum = hb_do_cksum((unsigned char *)addr, size);
	pinfo->boot_csum = csum;
	debug("---------, addr:%p, size:%u, 0x%x\n", addr, size, size);
	debug("boot_csum: 0x%x\n", csum);
	debug("---------\n");
}
static void bootinfo_cs_all(struct hb_info_hdr * pinfo)
{
	unsigned int csum;

	pinfo->info_csum = 0;
	csum = hb_do_cksum((unsigned char *)pinfo, sizeof(*pinfo));
	pinfo->info_csum = csum;
	debug("info_csum: 0x%x\n", csum);
}
static void bootinfo_update_spl(char * addr, unsigned int spl_size)
{
	struct hb_info_hdr *pinfo;
	unsigned int max_size = 0x6e00; /* CONFIG_SPL_MAX_SIZE in spl */

	debug("spl_size:%u, 0x%x\n", spl_size, spl_size);
	pinfo = (struct hb_info_hdr *) HB_BOOTINFO_ADDR;
	if (spl_size >= max_size)
		pinfo->boot_size = max_size;
	else
		pinfo->boot_size = spl_size;

	bootinfo_cs_spl(addr, spl_size, pinfo);
	bootinfo_cs_all(pinfo);
	write_bootinfo();
}

