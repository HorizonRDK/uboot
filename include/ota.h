/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __OTA_H_
#define __OTA_H_	1

#define NOR_SECTOR_SIZE (64*1024)

#define RECOVERY_PARTITION_NAME "recovery"

#define BOOT_PARTITION_FRONT_HALF_NAME "boot"
#define BOOT_PARTITION_NAME "boot"
#define BOOT_BAK_PARTITION_NAME "boot_b"

#define SYSTEM_PARTITION_FRONT_HALF_NAME "system"
#define SYSTEM_PARTITION_NAME "system"
#define SYSTEM_BAK_PARTITION_NAME "system_b"

#define PARTITION_SUFFIX_NAME ""
#define BAK_PARTITION_SUFFIX_NAME "_b"

#define MMC_WRITE_CMD "mmc write "

typedef enum {
    APP_SUCCESS_OFFSET,
    FIRST_TRY_OFFSET,
    FLASH_SUCCESS_OFFSET,
    UPDATE_SUCCESS_OFFSET,
} UP_FLAG;

typedef enum {
    SPL_OFFSET_FLAG = 0,
    UBOOT_OFFSET_FLAG,
    BOOT_OFFSET_FLAG,
    SYSTEM_OFFSET_FLAG,
    APP_OFFSET_FLAG,
    USERDATA_OFFSET_FLAG,
} PAR_OFFSET_FLAG;

extern char boot_partition[64];
extern char system_partition[64];

extern char hb_upmode[32];
extern char hb_bootreason[32];
extern char hb_partstatus;

char *printf_efiname(gpt_entry *pte);

unsigned int hex_to_char(unsigned int temp);

void uint32_to_char(unsigned int temp, char *s);

int get_partition_id(char *partname);

int ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
        char *const argv[]);

void ota_recovery_mode_set(bool upflag);

void ota_ab_boot_bak_partition(void);

unsigned int ota_check_update_success_flag(void);

void ota_upgrade_flag_check(char *up_mode, char *boot_reason);

int ota_download_and_upimage(cmd_tbl_t *cmdtp, int flag, int argc,
				char *const argv[]);

// bool hb_nor_ota_upflag_check(void);

#endif
