/*
 * (C) Copyright 2002
 * Rich Ireland, Enterasys Networks, rireland@enterasys.com.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _VEEPROM_H_
#define _VEEPROM_H_

#define VEEPROM_START_SECTOR (34)
#define VEEPROM_END_SECTOR (37)

#define VEEPROM_MACADDR_OFFSET      0
#define VEEPROM_MACADDR_SIZE        6
#define VEEPROM_RBFINDEX_OFFSET     6
#define VEEPROM_RBFINDEX_SIZE       1
#define VEEPROM_UPDATE_FLAG_OFFSET  7
#define VEEPROM_UPDATE_FLAG_SIZE    1
#define VEEPROM_IPADDR_OFFSET       8
#define VEEPROM_IPADDR_SIZE         4
#define VEEPROM_IPMASK_OFFSET       12
#define VEEPROM_IPMASK_SIZE         4
#define VEEPROM_IPGATE_OFFSET       16
#define VEEPROM_IPGATE_SIZE         4
#define VEEPROM_APPINDEX_OFFSET     20
#define VEEPROM_APPINDEX_SIZE       1
#define VEEPROM_PERI_FLAG_OFFSET    21
#define VEEPROM_PERI_FLAG_SIZE      1
#define VEEPROM_CORE_FLAG_OFFSET    22
#define VEEPROM_CORE_FLAG_SIZE      1
#define VEEPROM_SWP_FLAG_OFFSET     23
#define VEEPROM_SWP_FLAG_SIZE       1

#define VEEPROM_DUID_OFFSET         220
#define VEEPROM_DUID_SIZE           32

#define VEEPROM_MAX_SIZE			256

struct mmc *init_mmc_device(int dev, bool force_init);
int veeprom_init(void);

void veeprom_exit(void);

int veeprom_format(void);

int veeprom_read(int offset, char *buf, int size);

int veeprom_write(int offset, const char *buf, int size);

int veeprom_clear(int offset, int size);

int veeprom_dump(void);
#endif  /* _VEEPROM_H_ */
