
// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * Ext4fs support
 * made from existing cmd_ext2.c file of Uboot
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
 */

/*
 * Changelog:
 *	0.1 - Newly created file for ext4fs support. Taken from cmd_ext2.c
 *	        file in uboot. Added ext4fs ls load and write support.
 */

#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <linux/stat.h>
#include <malloc.h>
#include <fs.h>
#include <ota.h>


int do_download_and_upimage(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	return ota_download_and_upimage(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(dw, 2, 0, do_download_and_upimage,
	   "download and write binary file to gpt partition",
	   "<target name>\n"
	   "    - ubootbin: \n"
	   "           update uboot partition, using image u-boot.bin \n"
	   "           cmd: tftp 0x21000000 u-boot.bin;otawrite\n"
	   "    - uboot\n"
	   "           update uboot partition, using image uboot.img\n"
	   "           cmd: tftp 0x21000000 uboot.img;otawrite\n"
	   "    - boot\n"
	   "           update boot partition, using image boot.img\n"
	   "           cmd: tftp 0x21000000 boot.img;otawrite\n"
	   "    - disk\n"
	   "           update all partitions\n"
	   "           cmd: tftp 0x21000000 disk.img;otawrite\n"
	   "          [2020-08]\n"
);

int do_ota_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	return ota_write(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(otawrite, 5, 0, do_ota_write,
	   "write binary file to gpt partition",
	   "<partition name> <ddr addr> <image size> [emmc|nor|nand] \n"
	   "    - emmc partition name: \n"
	   "           [all | gpt-main | sbl | ddr | uboot | vbmeta \n"
	   "            | boot | kernel(vbmeta+boot) | system | app | gpt-backup]\n"
	   "    - nor partition name: \n"
	   "           [all | uboot | vbmeta | kernel | system | app]\n"
		"    - nand partition name: \n"
	   "           [all | bootloader | sys | rootfs | userdata]\n"
	   "    - image size: \n"
	   "           bytes size  [Example: 0x8000]\n"
	   "    - emmc|nor|nand: \n"
	   "          options, write emmc, nor or nand partition\n"
	   "          default: writing device depend on bootmode\n"
	   "    - example:\n"
	   "          otawrite uboot 0x4000000 0x100000\n"
	   "    - version: \n"
	   "          [2020-09]\n"
);
