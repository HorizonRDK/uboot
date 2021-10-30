// SPDX-License-Identifier: GPL
/*
 * Command for accessing w1 secure chip.
 *
 * Copyright (C) 2021 Horizon Robotics, Inc.
 */

#include <common.h>
#include <command.h>

#include "w1/aes.h"
#include "w1/aes_locl.h"
#include "w1/hobot_aes.h"
#include "w1/modes.h"
#include "w1/w1_ds28e1x_sha256.h"
#include "w1/w1_family.h"
#include "w1/w1_gpio.h"
#include "w1/w1.h"
#include "w1/w1_int.h"
#include "w1/x1_gpio.h"

#define  SECURE_KEY_TYPE  0x00000003
#define  HEADER_FIX       0x31764750
#define  SECURE_IC_ID     0x4B
#define  PACK_LEN         16
#define  SECURE_KEY_LEN   1024

enum pack_index {
    HEADER_INDEX = 0,
    TYPE_INDEX,
    LENGTH_INDEX,
    CRC_INDEX,
    DATA_INDEX,
    MAX_STATES = 0x0F
};

static int mark_register_cnt = 0;

static struct w1_master                 *master_total = NULL;
static struct w1_bus_master              bus_master;
static struct w1_gpio_platform_data      pdata_sf;

static int check_Crc(u16 crc_start, unsigned char *Data, unsigned int len)
{
    unsigned int mCrc = crc_start;
    unsigned int i, j;

    for (j = 0; j < len; j++) {
        mCrc = mCrc ^ (u16)(Data[j]) << 8;
        for (i = 8; i != 0; i--) {
            if (mCrc & 0x8000)
                mCrc = mCrc << 1 ^ 0x1021;
            else
                mCrc = mCrc << 1;
        }
    }
    return mCrc;
}

static int w1_init_setup(void)
{
    if (w1_ds28e1x_init() < 0) {
        printf("ds28e1x init fail !!!\n");
        return -1;
    }

    memset(&bus_master, 0x0, sizeof(struct w1_bus_master));
    memset(&pdata_sf, 0x0, sizeof(struct w1_gpio_platform_data));
    master_total = w1_gpio_probe(&bus_master, &pdata_sf);

    if (!master_total) {
        printf("w1_gpio_probe fail !!!\n");
        return -1;
    }

    if (w1_process(master_total) < 0) {
        printf("ds28e1x read ID fail !!!\n");
        return -1;
    }

    if (w1_master_setup_slave(master_total, SECURE_IC_ID, NULL, NULL) < 0) {
        printf("ds28e1x setup_device fail !!!\n");
        return -1;
    }

    return 0;
}

static int do_burn_w1(cmd_tbl_t *cmdtp, int flag, int argc,
        char * const argv[])
{
	int ret = 0, has_auth;
	u32  offset;
	u8 *addr = (u8 *)&offset;
	unsigned int header, type, d_length, crc, c_crc;
	unsigned char package[SECURE_KEY_LEN] = {0};
	unsigned char secure_key[SECURE_KEY_LEN] = {0};
	unsigned int *p_pack = (unsigned int *)package;
	unsigned char key_note[HOBOT_AES_BLOCK_SIZE] = {0x3f, 0x48, 0x15,
        0x16, 0x6f, 0xae, 0xd2, 0xa6, 0xe6, 0x27, 0x15, 0x69, 0x09,
        0xcf, 0x7a, 0x3c};
	unsigned char real_key[32] = {0};

	if (!mark_register_cnt)
	    ret = w1_init_setup();

	if (ret < 0) {
		printf("burn_key_w1_init_setup_error\n");
		printf("burn_key_failed\n");
		return 0;
	}
	mark_register_cnt = 1;

	offset = simple_strtoul(argv[1], NULL, 16);    /* secure data addr in ddr*/

	memcpy(&package[0], addr, sizeof(package));

	header = *p_pack;
	type = *(p_pack + TYPE_INDEX);
	d_length = *(p_pack + LENGTH_INDEX);
	crc = *(p_pack + CRC_INDEX);

	if (header != HEADER_FIX) {
		printf("burn_key_header_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	if (type != SECURE_KEY_TYPE) {
		printf("burn_key_type_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	if (d_length <= SECURE_KEY_LEN - PACK_LEN) {
		memcpy(&secure_key[0], (unsigned char *)(p_pack + DATA_INDEX),
            d_length);
	} else {
		printf("burn_key_length_error, length > %dbyte\n",
            SECURE_KEY_LEN - PACK_LEN);
		printf("burn_key_failed\n");
		return 0;
	}

	c_crc = check_Crc(0, &package[PACK_LEN], d_length);
	if (crc != c_crc) {
		printf("burn_key_crc_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	ret = w1_master_is_write_auth_mode(master_total, SECURE_IC_ID, &has_auth);
	if (ret != 0) {
		printf("w1_master_is_write_auth_mode failed\n");
		printf("burn_key_failed\n");
		return 0;
	}

	/* decrypt to realy key */
	hb_aes_decrypt((char *)&secure_key[32], (char *)key_note,
        (char *)real_key, 32);
	memcpy(&secure_key[32], real_key, 32);

	/* load the secure key only */
	ret = w1_master_load_key(master_total, SECURE_IC_ID,
        (char *)secure_key, NULL);
	if (ret != 0) {
		printf("burn_key_w1_master_load_key_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	/* load the usr_data */
	ret = w1_master_auth_write_usr_mem(master_total, SECURE_IC_ID,
	    (char *)secure_key);
	if (ret != 0) {
		printf("burn_key_w1_master_auth_write_usr_mem_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	if (has_auth) {
		ret = w1_master_auth_write_block_protection(master_total,
            SECURE_IC_ID, real_key);
		if (ret != 0) {
			printf("w1_master_auth_write_block_protection\n");
			printf("burn_key_failed\n");
			return 0;
		}
	} else {
		/* first set the auth mode */
		ret = w1_master_set_write_auth_mode(master_total, SECURE_IC_ID);
		if (ret != 0) {
			printf("burn_w1_master_set_write_auth_mode_error\n");
			printf("burn_key_failed\n");
			return 0;
		}
	}

	printf("burn_key_succeeded\n");

	return 0;
}
static int do_get_w1_sid(cmd_tbl_t *cmdtp, int flag, int argc,
        char * const argv[])
{
    unsigned char sec_id[8] = {0};
    int i = 0;
    int ret = 0;
    unsigned char total = 0;

    if (!mark_register_cnt) {
        ret = w1_init_setup();
        if (!ret)
            mark_register_cnt = 1;
    }

    if (mark_register_cnt)
        w1_master_get_rom_id(master_total, SECURE_IC_ID, (char *)sec_id);

    printf("w1_gid: ");
    for (i = 0; i < 8; i++) {
        printf("%d ", sec_id[i]);
        total += sec_id[i];
    }
    printf("%d ", total);
    printf("end\n");

	return 0;
}

U_BOOT_CMD(
    w1, 2, 0, do_burn_w1,
    "burn w1 secure chip",
    "burn addr\r\n"
    ""
);

U_BOOT_CMD(
    w1_gid, 2, 0, do_get_w1_sid,
    "get secret-ic rom-id data",
    "w1_gid \r\n"
    ""
);
