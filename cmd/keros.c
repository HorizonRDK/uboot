// SPDX-License-Identifier: GPL
/*
 * Command for accessing keros secure chip.
 *
 * Copyright (C) 2021 Horizon Robotics, Inc.
 */

#include <common.h>
#include <command.h>

#include "keros/keros.h"

#define  HEADER_FIX       0x31764750
#define  SECURE_KEY_TYPE  0x00000003
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

static int check_Crc(uint16_t crc_start, unsigned char *Data, unsigned int len)
{
    uint32_t mCrc = crc_start;
    uint32_t i, j;

    for (j = 0; j < len; j++) {
        mCrc = mCrc ^ (uint16_t)(Data[j]) << 8;
        for (i = 8; i != 0; i--) {
            if (mCrc & 0x8000)
                mCrc = mCrc << 1 ^ 0x1021;
            else
                mCrc = mCrc << 1;
        }
    }
    return mCrc;
}

static int do_burn_keros(cmd_tbl_t *cmdtp, int flag, int argc,
        char * const argv[])
{
	int ret = 0;
	unsigned long offset;
	uint8_t page, encrytion;
	uint32_t old_password, new_password;
	uint32_t header, type, d_length, crc, c_crc;
	uint8_t  package[SECURE_KEY_LEN] = {0};
	uint8_t  secure_key[SECURE_KEY_LEN] = {0};
	uint32_t *p_pack = (uint32_t *)package;

	if (!mark_register_cnt)
        ret = keros_init();

	if (ret < 0) {
		printf("keros init failed\n");
		printf("burn_key_failed\n");
		return 0;
	}
	mark_register_cnt = 1;

	offset = simple_strtoul(argv[1], NULL, 16);    /* secure data addr in ddr*/

	memcpy(&package[0],(uint8_t *)offset,sizeof(package));

	header = *p_pack;
	type = *(p_pack + TYPE_INDEX);
	d_length =  *(p_pack + LENGTH_INDEX);
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
		page = *(p_pack + DATA_INDEX);
		encrytion = *(p_pack + DATA_INDEX + 1);
		old_password = *(p_pack + DATA_INDEX + 2);
		new_password = *(p_pack + DATA_INDEX + 3);
		memcpy(&secure_key[0], (uint8_t *)(p_pack + DATA_INDEX + 4),
            d_length - 4 * 4);
	} else {
		printf("burn_key_length_error, length > %dbyte\n",
            SECURE_KEY_LEN - PACK_LEN);
		printf("burn_key_failed\n");
		return 0;
	}

	debug("page： %d\n", page);
	debug("encrytion: %d\n", encrytion);
	debug("old password: %d\n", old_password);
	debug("new password: %d\n", new_password);
	debug("content:\n");
	for (int i = 0; i < d_length; ++i) {
		debug("%x", secure_key[i]);
	}
	debug("\n");

	c_crc = check_Crc(0, &package[PACK_LEN], d_length);
	if (crc != c_crc) {
		printf("burn_key_crc_error\n");
		printf("burn_key_failed\n");
		return 0;
	}

	ret = keros_authentication();
	if (ret != 0) {
		printf("keros authentication failed\n");
		printf("burn_key_failed\n");
		return 0;
	}

	ret = keros_pwchg(page, old_password, new_password);
	if (ret != 0) {
		printf("keros password chang faild\n");
		printf("burn_key_failed\n");
	}

	/* load the secure key only */
	ret = keros_write_key(new_password, page, secure_key, encrytion);
	if (ret != 0) {
		printf("write key to eeprom failed\n");
		printf("burn_key_failed\n");
		return 0;
	}

	printf("burn_key_succeeded\n");
	return 0;
}

U_BOOT_CMD(
	keros, 2, 0, do_burn_keros,
	"burn keros secure chip",
	"burn addr \r\n"
	""
);
