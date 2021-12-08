/*
 * (C) Copyright 2002
 * Rich Ireland, Enterasys Networks, rireland@enterasys.com.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _VEEPROM_H_
#define _VEEPROM_H_

/* define copy from include/veeprom.h in uboot */
#define VEEPROM_START_SECTOR (34)
#define VEEPROM_END_SECTOR (37)

#define NOR_VEEPROM_START_SECTOR (0)
#define NOR_VEEPROM_END_SECTOR (3)

#define VEEPROM_BOARD_ID_OFFSET		0
#define VEEPROM_BOARD_ID_SIZE		2
#define VEEPROM_MACADDR_OFFSET      2
#define VEEPROM_MACADDR_SIZE        6
#define VEEPROM_UPDATE_FLAG_OFFSET  8
#define VEEPROM_UPDATE_FLAG_SIZE    1
#define VEEPROM_RESET_REASON_OFFSET	9
#define VEEPROM_RESET_REASON_SIZE	8
#define VEEPROM_IPADDR_OFFSET       17
#define VEEPROM_IPADDR_SIZE         4
#define VEEPROM_IPMASK_OFFSET       21
#define VEEPROM_IPMASK_SIZE         4
#define VEEPROM_IPGATE_OFFSET       25
#define VEEPROM_IPGATE_SIZE         4
#define VEEPROM_UPDATE_MODE_OFFSET  29
#define VEEPROM_UPDATE_MODE_SIZE    8
#define VEEPROM_ABMODE_STATUS_OFFSET  37
#define VEEPROM_ABMODE_STATUS_SIZE    1
#define VEEPROM_COUNT_OFFSET  		38
#define VEEPROM_COUNT_SIZE    		1
#define VEEPROM_SOMID_OFFSET  		39
#define VEEPROM_SOMID_SIZE    		1
#define VEEPROM_PERI_PLL_OFFSET     40
#define VEEPROM_PERI_PLL_SIZE       16
/* 56-137 reserved */
#define VEEPROM_UBUNTU_MAGIC_OFFSET 137
#define VEEPROM_UBUNTU_MAGIC_SIZE   4
#define VEEPROM_DUID_OFFSET         220
#define VEEPROM_DUID_SIZE           32


#define VEEPROM_MAX_SIZE			256

#define UPMODE_AB "AB"
#define UPMODE_GOLDEN "golden"

#define REASON_RECOVERY "recovery"
#define REASON_NORMAL "normal"
#define REASON_UBOOT "uboot"
#define REASON_BOOT "boot"
#define REASON_SYSTEM "system"
#define REASON_ALL "all"
#define UBUNTU_MAGIC "UBTU"

struct mmc *init_mmc_device(int dev, bool force_init);
int veeprom_init(void);

void veeprom_exit(void);

int veeprom_format(void);

int veeprom_read(int offset, char *buf, int size);

int veeprom_write(int offset, const char *buf, int size);

int veeprom_clear(int offset, int size);

int veeprom_dump(void);
#endif  /* _VEEPROM_H_ */
