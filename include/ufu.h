/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * f_ufu.h -- TCL USB Firmware Update gadget
 *
 * Copyright (C) 2020 Horizon Robotics
 *                    Author: Jianghe Xu<jianghe.xu@horizon.ai>
 */
#ifndef __UFU_H__
#define __UFU_H__

#include <common.h>
#include <part.h>
#include <linux/usb/composite.h>

#ifdef CONFIG_USB_FUNCTION_UFU

#define UFU_RUN_COMMNAD_MAX_LENGTH 1024		/* 1024 bytes at most */
#define UFU_MAX_BLK_WRITE 16384
#define IS_UFU_UMS_DNL(name)	(!strncmp((name), "ufu_ums_dnl", 11))

enum ufu_cmd {
	UFU_CMD_FIRMWARE		= 0xE8,
};

enum ufu_firmware_subcmd {
	UFU_SUBCMD_DOWNLOAD_KEEP	= 0x1,
	UFU_SUBCMD_GET_RESULT		= 0x2,
	UFU_SUBCMD_GET_STATE		= 0x3,
	UFU_SUBCMD_DOWNLOAD_END		= 0x4,
	UFU_SUBCMD_LOADINFO		= 0x5,
	UFU_SUBCMD_RUN			= 0x6,
	UFU_SUBCMD_IDENTIFY		= 0x7,
};

enum ufu_rc {
	UFU_RC_ERROR			= -1,
	UFU_RC_CONTINUE			= 0,
	UFU_RC_FINISHED			= 1,
	UFU_RC_UNKNOWN_CMND		= 2,
};

enum ufu_error_code {
	UFU_SUCCESS			= 0,
	UFU_ERR_MD5			= 1,
	UFU_ERR_INVALID_PARAM		= 2,
	UFU_ERR_RUNMD			= 3,
	UFU_ERR_IMG_FMT			= 4,
	UFU_ERR_NOT_IMPLEMENT		= 5,
	UFU_ERR_DEVICE_TYPE		= 6,
	UFU_ERR_MAX			= 7,
};

struct load_info {
	u32		addr;
	u32		size;
	u8		md5[16];
};

struct f_ufu {
	/* function part reuse f_mass_storage function */

	struct load_info	loadinfo;
	unsigned int		offset;
	unsigned int		chunks;
	u8			download_finish;
	u8			flash_finish;
	u8			md5_pass;

	char			command[UFU_RUN_COMMNAD_MAX_LENGTH];
};

struct f_ufu *get_ufu(void);
#else
#define IS_UFU_UMS_DNL(name)	0

struct fsg_buffhd;
struct fsg_dev;
struct fsg_common;
struct fsg_config;

static struct usb_descriptor_header *ufu_fs_function[];
static struct usb_descriptor_header *ufu_hs_function[];

static inline int ufu_cmd_process(struct fsg_common *common,
				    struct fsg_buffhd *bh, int *reply)
{
	return -EPERM;
}
#endif

struct ufu_device {
	struct ums *ums;
	int ums_cnt;
};

#endif /* __UFU_H__ */
