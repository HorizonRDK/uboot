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
#include <mtd.h>
#include <veeprom.h>
#include <ota.h>
#include <hb_info.h>
#include <asm/arch/hb_pmu.h>
#include <asm/io.h>

#ifdef CONFIG_CMD_OTA_WRITE
static void bootinfo_update_spl(char * addr, unsigned int spl_size);
static void bootinfo_update_uboot(unsigned int uboot_size);
#endif /*CONFIG_CMD_OTA_WRITE*/

static int curr_device = 0;
extern struct spi_flash *flash;
extern bool recovery_sys_enable;
extern char net_boot_file_name[1024];

char boot_partition[64] = BOOT_PARTITION_NAME;
char system_partition[64] = SYSTEM_PARTITION_NAME;

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
#ifdef CONFIG_CMD_OTA_WRITE
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
#endif /*CONFIG_CMD_OTA_WRITE*/

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


int ota_download_and_upimage(cmd_tbl_t *cmdtp, int flag, int argc,
				char *const argv[])
{
	char *partition_name = NULL;
	char *file_name = NULL;
	char cmd[256] = { 0 };

	if (argc != 2) {
		printf("error: dw cmd need parameter ! \n");
		return CMD_RET_USAGE;
	}

	if (strcmp(argv[1], "ubootbin") == 0) {
		partition_name = "uboot";
		file_name = "u-boot.bin";
	} else if (strcmp(argv[1], "uboot") == 0) {
		partition_name = "uboot";
		file_name = "uboot.img";
	} else if (strcmp(argv[1], "kernel") == 0) {
		partition_name = "kernel";
		file_name = "kernel.img";
	} else if (strcmp(argv[1], "disk") == 0) {
		partition_name = "all";
		file_name = "disk.img";
	} else {
		printf("error: parameter %s not support! \n", argv[1]);
		return CMD_RET_USAGE;
	}

	/* tftp load image */
	snprintf(cmd, sizeof(cmd), "tftp 0x21000000 %s", file_name);
	if (run_command(cmd, 0) != 0) {
		printf("tftp load %s failed\n", file_name);
		return CMD_RET_FAILURE;
	}

	/* write image to partition */
	snprintf(cmd, sizeof(cmd), "otawrite %s ${fileaddr} ${filesize}",
		partition_name);
	if (run_command(cmd, 0) != 0) {
		return CMD_RET_FAILURE;
	}

	return 0;
}

#ifdef CONFIG_CMD_OTA_WRITE
static int ota_flash_update_image(char *flash_type, char *partition,
								  char *addr, unsigned int bytes)
{
	char command[64];
	struct mtd_info *mtd;
	if (!strcmp("all", partition)) {
		mtd_probe_devices();
		mtd = __mtd_next_device(0);
		/* Ensure all devices (and their partitions) are probed */
		if (!mtd || list_empty(&(mtd->partitions))) {
			partition = "";
			printf("Erasing all Flash!\n");
		}
	}
	snprintf(command, sizeof(command), "burn_flash %s %s %s %x",
										flash_type, partition, addr, bytes);

	return run_command(command, 0);
}

static int ota_mmc_update_image(char *name, char *addr, unsigned int bytes)
{
	unsigned int start_lba = 0, end_lba = 0, kernel_end = 0;
	char command[256];
	char lba_size[64] = { 0 };
	char *s;
	int ret;
	unsigned int sector = (bytes + 511)/512;
	unsigned int part_size;
	void *realaddr;

	/* align 512byte */
	if (bytes % 512)
	memset(addr + bytes, 0, 512 - (bytes % 512));

	if (strcmp(name, "gpt-main") == 0) {
		printf("in gpt-main\n");
		if (bytes != 34*512) {
			printf("Error: gpt-main size(%x) is not equal to 0x%x\n",
					bytes, 34*512);
			return CMD_RET_FAILURE;
		}
		snprintf(command, sizeof(command), "%s %s 0 %x",
				 MMC_WRITE_CMD, addr, 34);
	} else if (strcmp(name, "gpt-backup") == 0) {
		printf("in gpt-backup\n");
		if (bytes != 33*512) {
			printf("Error: gpt-backup size(%x) is not equal to 0x%x\n",
					bytes, 33*512);
			return CMD_RET_FAILURE;
		}
		get_patition_lba("userdata", &start_lba, &end_lba);
		uint32_to_char(end_lba+1, lba_size);
		snprintf(command, sizeof(command), "%s %s %s %x",
				 MMC_WRITE_CMD, addr, lba_size, 33);
	} else if (strcmp(name, "all") == 0) {
		printf("in all\n");

		snprintf(command, sizeof(command), "%s %s 0 %x",
				 MMC_WRITE_CMD, addr, sector);
	} else if (strcmp(name, "kernel") == 0) {
		get_patition_lba("vbmeta", &start_lba, &end_lba);
		get_patition_lba("boot", &end_lba, &kernel_end);
		part_size = kernel_end - start_lba + 1;
		if (start_lba == kernel_end) {
			printf("Error: partition start_lba: %d, end_lba:%d!\n",
			       start_lba, kernel_end);
			return CMD_RET_FAILURE;
		}

		if (sector > part_size) {
			printf("Error: image more than partiton size %02x \n",
					part_size * 512);
			return CMD_RET_FAILURE;
		}

		snprintf(command, sizeof(command), "%s %s %x %x",
				 MMC_WRITE_CMD, addr, start_lba, sector);
	} else {
		get_patition_lba(name, &start_lba, &end_lba);
		part_size = end_lba - start_lba + 1;
		if (start_lba == end_lba) {
			printf("Error: partition start_lba: %d, end_lba:%d!\n",
			       start_lba, end_lba);
			return CMD_RET_FAILURE;
		}

		if (sector > part_size) {
			printf("Error: image more than partiton size %02x \n",
				    part_size * 512);
			return CMD_RET_FAILURE;
		}

		snprintf(command, sizeof(command), "%s %s %x %x",
				 MMC_WRITE_CMD, addr, start_lba, sector);

		if (strcmp(name, "sbl") == 0) {
			realaddr = (void *)simple_strtoul(addr, NULL, 16);
			bootinfo_update_spl(realaddr, bytes);
		} else if (strcmp(name, "uboot") == 0) {
			bootinfo_update_uboot(bytes);
		}
	}

	printf("command : %s \n", command);
	ret = run_command_list(command, -1, 0);
	if (ret < 0)
		return ret;

	return ret;
}

int ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	char *file_name = NULL, *filesize = NULL, *fileaddr = NULL;
	char *ep = NULL, *ptr = NULL, *flash_type = NULL;
	char partition_name[256] = { 0 };
	int ret;
	unsigned int boot_mode, bytes;

	if ((argc != 1) && (argc != 4) && (argc != 5)) {
		printf("error: parameter number %d not support! \n", argc);
		return CMD_RET_USAGE;
	}

	/* get bootmode */
	if (argc == 5) {
		if (strcmp(argv[4], "emmc") == 0) {
			boot_mode = PIN_2ND_EMMC;
		} else if (strcmp(argv[4], "nor") == 0) {
			boot_mode = PIN_2ND_NOR;
			flash_type = "nor";
		} else if (strcmp(argv[4], "nand") == 0) {
			boot_mode = PIN_2ND_NAND;
			flash_type = "nand";
		} else {
			printf("error: parameter %s not support! \n", argv[4]);
			return CMD_RET_USAGE;
		}
	} else {
		boot_mode = hb_boot_mode_get();
	}

	/* get partition name, file size and file addr */
	if (argc == 1) {
		file_name = net_boot_file_name;
		printf("file_name: %s\n", net_boot_file_name);

		if (strcmp(file_name, "u-boot.bin") == 0) {
			snprintf(partition_name, sizeof(partition_name), "uboot");
		} else if (strcmp(file_name, "disk.img") == 0) {
			snprintf(partition_name, sizeof(partition_name), "all");
		} else {
			ptr = strchr(file_name, '.');
			if (ptr != NULL) {
				strncpy(partition_name, file_name, ptr - file_name);
				printf("partition_name : %s\n", partition_name);
			} else {
				printf("error: image name %s not support ! \n",
					file_name);
				return CMD_RET_FAILURE;
			}
		}

		filesize = env_get("filesize");
		fileaddr = env_get("fileaddr");

		bytes = simple_strtoul(filesize, &ep, 16);
	} else {
		snprintf(partition_name, sizeof(partition_name), "%s", argv[1]);

		fileaddr = argv[2];
		bytes = simple_strtoul(argv[3], &ep, 16);
		if (ep == argv[3] || *ep != '\0')
			return CMD_RET_USAGE;
	}

	/* update image */
	if (boot_mode == PIN_2ND_EMMC)
		ret = ota_mmc_update_image(partition_name, fileaddr, bytes);
	else
		ret = ota_flash_update_image(flash_type, partition_name, fileaddr, bytes);

	if (ret == 0)
		printf("ota update image success!\n");
	else
		printf("Error: ota update image failed!\n");

	return ret;
}
#endif /*CONFIG_CMD_OTA_WRITE*/

static void ota_error(char *partition)
{
	printf("*************************************************\n");
	printf("Error: ota upgrade %s partiton failed! \n", partition);
	printf("*************************************************\n");
}

void ota_recovery_mode_set(bool upflag)
{
	//char boot_reason[16] = "recovery";
	int boot_mode = hb_boot_mode_get();

	if (recovery_sys_enable) {
		if (upflag) {
			veeprom_write(VEEPROM_RESET_REASON_OFFSET, RECOVERY_PARTITION_NAME,
				VEEPROM_RESET_REASON_SIZE);
		}

		if (boot_mode == PIN_2ND_EMMC)
			snprintf(boot_partition, sizeof(boot_partition), RECOVERY_PARTITION_NAME);
	}
}

static void ota_normal_boot(bool root_flag, bool boot_flag)
{
	int boot_mode = hb_boot_mode_get();

	if ((boot_mode == PIN_2ND_NOR) || (boot_mode == PIN_2ND_NAND)) {
		return;
	}

	if (boot_flag == 1)
		snprintf(boot_partition, sizeof(boot_partition),
			BOOT_BAK_PARTITION_NAME);

	if (root_flag == 1)
		snprintf(system_partition, sizeof(system_partition),
			SYSTEM_BAK_PARTITION_NAME);
}

static void ota_uboot_upflag_check(char up_flag, char *upmode)
{
	bool update_success = (up_flag >> UPDATE_SUCCESS_OFFSET) & 0x1;

	if ((update_success == 0) && (strcmp(upmode, UPMODE_GOLDEN) == 0)) {
		/* when update uboot failed in golden mode, into recovery system */
		ota_recovery_mode_set(true);
	}
}

static void ota_kernel_upflag_check(char *upmode, char up_flag,
	bool part_status)
{
	bool flash_success, first_try, app_success;
	bool boot_flag = part_status;

	/* update flag */
	flash_success = (up_flag >> FLASH_SUCCESS_OFFSET) & 0x1;
	first_try = (up_flag >> FIRST_TRY_OFFSET) & 0x1;
	app_success = up_flag & 0x1;

	if (flash_success == 0) {
		boot_flag = part_status ^ 1;
	} else if (first_try == 1) {
		up_flag = up_flag & 0xd;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);
		return;
	} else if (app_success == 0) {
			boot_flag = part_status ^ 1;
	}

	/* If update failed, using backup partition */
	if (boot_flag != part_status) {
		ota_error(BOOT_PARTITION_NAME);

		up_flag = up_flag & 0x7;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

		if (strcmp(upmode, UPMODE_GOLDEN) == 0) {
			/* update failed, enter recovery mode */
			ota_recovery_mode_set(true);
		} else if (boot_flag == 1) {
			snprintf(boot_partition, sizeof(boot_partition), BOOT_BAK_PARTITION_NAME);
		}
	}
}

static void ota_system_upflag_check(char *upmode,
	char up_flag, bool part_status)
{
	bool flash_success, first_try, app_success;
	bool boot_flag = part_status;

	/* update flag */
	flash_success = (up_flag >> FLASH_SUCCESS_OFFSET) & 0x1;
	first_try = (up_flag >> FIRST_TRY_OFFSET) & 0x1;
	app_success = up_flag & 0x1;

	if (flash_success == 0) {
		boot_flag = part_status ^ 1;
	} else if (first_try == 1) {
		up_flag = up_flag & 0xd;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);
	} else if(app_success == 0) {
			boot_flag = part_status^1;
	}

	/* If update failed, using backup partition */
	if (boot_flag != part_status) {
		ota_error(SYSTEM_PARTITION_NAME);

		up_flag = up_flag & 0x7;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

		if (strcmp(upmode, UPMODE_GOLDEN) == 0) {
			/* update failed, enter recovery mode */
			ota_recovery_mode_set(true);
		} else if (boot_flag == 1) {
			snprintf(system_partition, sizeof(system_partition),
				SYSTEM_BAK_PARTITION_NAME);
		}
	}
}

static void ota_reverse_ab(void)
{
	char partstatus = 0;
	char upmode[16] = { 0 };

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
		VEEPROM_UPDATE_MODE_SIZE);
	if (strcmp(upmode, UPMODE_AB) == 0) {
		veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, &partstatus,
				VEEPROM_ABMODE_STATUS_SIZE);
		partstatus ^= 0xff;
		veeprom_write(VEEPROM_ABMODE_STATUS_OFFSET, &partstatus,
				VEEPROM_ABMODE_STATUS_SIZE);
	}
}

static void ota_all_update(char *upmode, char up_flag, bool boot_stat,
	bool root_stat)
{
	bool flash_success, app_success, update_success;
	bool root_flag = root_stat;
	bool boot_flag = boot_stat;
	uint32_t count = 0;
	bool count_pmu_flag = false;
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;

	/* uboot flag check */
	ota_uboot_upflag_check(up_flag, upmode);

	/* update flag */
	update_success = (up_flag >> UPDATE_SUCCESS_OFFSET) & 0x1;
	flash_success = (up_flag >> FLASH_SUCCESS_OFFSET) & 0x1;
	app_success = up_flag & 0x1;
	veeprom_read(VEEPROM_COUNT_OFFSET, (char *)&count, VEEPROM_COUNT_SIZE);
	if (count == 'E') {
		/* read count value from pmu register */
		count_pmu_flag = true;
		count = (readl(HB_PMU_SW_REG_23) >> 16) & 0xffff;
	}
	if (flash_success == 0 || update_success == 0) {
		DEBUG_LOG("%s:%d:update_flag:%d\n", __func__, __LINE__, update_success);
		boot_flag = boot_stat ^ 1;
		root_flag = root_stat ^ 1;
	} else if (count < bootinfo->reserved[0]) {
		count = count + 2;
		if (count_pmu_flag == true) {
			count = (count << 16) | (readl(HB_PMU_SW_REG_23) & 0xffff);
			writel(count, HB_PMU_SW_REG_23);
		} else {
			veeprom_write(VEEPROM_COUNT_OFFSET, (char *)&count,
				VEEPROM_COUNT_SIZE);
		}
	} else if (bootinfo->reserved[0] == 0) {
		DEBUG_LOG("skip ota count check\n");
	} else if(app_success == 0) {
		boot_flag = boot_stat ^ 1;
		root_flag = root_stat ^ 1;
	}

	/* If update failed, using backup partition */
	if (boot_flag != boot_stat) {
		ota_error(REASON_ALL);

		up_flag = up_flag & 0x7;
		veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

		if (strcmp(upmode, UPMODE_GOLDEN) == 0) {
			/* update failed, enter recovery mode */
			ota_recovery_mode_set(true);
		} else {
			if (boot_flag == 1) {
				snprintf(boot_partition,
					sizeof(boot_partition), BOOT_BAK_PARTITION_NAME);
			} else {
				snprintf(boot_partition,
					sizeof(boot_partition), BOOT_PARTITION_NAME);
			}

			if (root_flag == 1) {
				snprintf(system_partition,
					sizeof(system_partition), SYSTEM_BAK_PARTITION_NAME);
			} else {
				snprintf(system_partition,
					sizeof(system_partition), SYSTEM_PARTITION_NAME);
			}
			ota_reverse_ab();
		}
	}
}

void ota_ab_boot_bak_partition(void)
{
	char partstatus;
	bool boot_flag, part_status;

	veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, &partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	/* get system backup partition id */
	part_status = (partstatus >> SYSTEM_OFFSET_FLAG) & 0x1;
	boot_flag = part_status ^ 1;

	if (boot_flag == 1)
		snprintf(system_partition, sizeof(system_partition),
			SYSTEM_BAK_PARTITION_NAME);
	/* get kernel backup partition id */
	part_status = (partstatus >> BOOT_OFFSET_FLAG) & 0x1;
	boot_flag = part_status ^ 1;

	if (boot_flag == 1)
		snprintf(boot_partition, sizeof(boot_partition), BOOT_BAK_PARTITION_NAME);
	ota_reverse_ab();
	DEBUG_LOG("boot parition: %s, system partition: %s\n",
			boot_partition, system_partition);
}

void ota_upgrade_flag_check(char *upmode, char *boot_reason)
{
	char up_flag, partstatus;
	bool root_stat, boot_stat;

	/* get upflag and partition status */
	veeprom_read(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

	veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, &partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	hb_partstatus = partstatus;

	/* get rootfs and kernel partition status */
	boot_stat = (partstatus >> BOOT_OFFSET_FLAG) & 0x1;
	root_stat = (partstatus >> SYSTEM_OFFSET_FLAG) & 0x1;

	/* update: setting delay 0 */
	if (strcmp(boot_reason, REASON_NORMAL) != 0)
		env_set("bootdelay", "0");

	/* normal boot */
	if ((strcmp(upmode, UPMODE_AB) == 0) &&
		((strcmp(boot_reason, REASON_SYSTEM) == 0) ||
		(strcmp(boot_reason, REASON_BOOT) == 0) ||
		(strcmp(boot_reason, REASON_ALL) == 0) ||
		strcmp(boot_reason, REASON_NORMAL) == 0))  {
			ota_normal_boot(root_stat, boot_stat);
	}

	/* check flag: uboot, kernel, system or all */
	if (strcmp(boot_reason, REASON_UBOOT) == 0) {
		ota_uboot_upflag_check(up_flag, upmode);
	} else if (strcmp(boot_reason, REASON_BOOT) == 0) {
		ota_kernel_upflag_check(upmode, up_flag, boot_stat);
	} else if (strcmp(boot_reason, REASON_SYSTEM) == 0) {
		ota_system_upflag_check(upmode, up_flag, root_stat);
	} else if (strcmp(boot_reason, REASON_ALL) == 0) {
		ota_all_update(upmode, up_flag, boot_stat, root_stat);
	}
	DEBUG_LOG("boot partition: %s, system partition: %s\n",
				boot_partition, system_partition);
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
#ifdef CONFIG_CMD_OTA_WRITE
static void write_bootinfo(void)
{
	int ret;
	char cmd[256] = {0};

	snprintf(cmd, sizeof(cmd), "mmc write 0x%x 0 0x1", HB_BOOTINFO_ADDR);
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
	csum = hb_do_cksum((unsigned char *)pinfo, BOOT_INFO_CHECK_SIZE);
	pinfo->info_csum = csum;
	debug("info_csum: 0x%x\n", csum);
}
static void bootinfo_update_spl(char * addr, unsigned int spl_size)
{
	struct hb_info_hdr *pinfo;
	unsigned int max_size = 0x40000; /* CONFIG_SPL_MAX_SIZE in spl */

	debug("spl_size:%u, 0x%x\n", spl_size, spl_size);
	pinfo = (struct hb_info_hdr *) HB_BOOTINFO_ADDR;
	if (spl_size >= max_size)
		pinfo->boot_size = max_size;
	else
		pinfo->boot_size = spl_size;

	bootinfo_cs_spl(addr, pinfo->boot_size, pinfo);
	bootinfo_cs_all(pinfo);
	write_bootinfo();
}

static void bootinfo_update_uboot(unsigned int uboot_size)
{
	struct hb_info_hdr *pinfo;
	unsigned int max_size = 0x200000; /* CONFIG_SPL_MAX_SIZE in spl */

	debug("spl_size:%u, 0x%x\n", uboot_size, uboot_size);
	pinfo = (struct hb_info_hdr *) HB_BOOTINFO_ADDR;
	if (uboot_size >= max_size)
		pinfo->other_img[0].img_size = max_size;
	else
		pinfo->other_img[0].img_size = uboot_size;

	bootinfo_cs_all(pinfo);
	write_bootinfo();
}
#endif /*CONFIG_CMD_OTA_WRITE*/
