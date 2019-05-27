/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __OTA_H_
#define __OTA_H_	1

char *printf_efiname(gpt_entry *pte);

int get_patition_start_lba(char *partname);

int get_patition_end_lba(char *partname);

unsigned int hex_to_char(unsigned int temp);

void uint32_to_char(unsigned int temp, char *s);

int get_partition_id(char *partname);

int ota_update_image(char *name, char *addr, unsigned int bytes);

int ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[]);


#endif
