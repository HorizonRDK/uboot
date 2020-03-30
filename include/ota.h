/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __OTA_H_
#define __OTA_H_	1

#define NOR_SECTOR_SIZE (64*1024)

#define NOR_UBOOT_ADDR 			(0x40000)
#define NOR_UBOOT_MAX_SIZE 		(0x100000)
#define NOR_KERNEL_ADDR 		(0x140000)
#define NOR_KERNEL_MAX_SIZE 	(0xEC0000)
#define NOR_ROOTFS_ADDR 		(0x1000000)
#define NOR_ROOTFS_MAX_SIZE 	(0x1000000)
#define NOR_APP_ADDR 			(0x2000000)
#define NOR_APP_MAX_SIZE 		(0x2000000)

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

// bool hb_nor_ota_upflag_check(void);

#endif
