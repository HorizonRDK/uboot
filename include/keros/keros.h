/*****************************************************************************
 * KEROS Interface driver
 *
 *
 * Copyright(C) CHIPSBRAIN GLOBAL CO., Ltd.
 * All rights reserved.
 *
 * File Name    : keros_interface.h
 * Author       : ARES HA
 *
 * Version      : V0.3
 * Date         : 2015.09.08
 * Description  : Keros Interface Header
 ****************************************************************************/
#ifndef __KEROS_INTERFACE_H__
#define __KEROS_INTERFACE_H__

#include "keros/keros_lib_1_8v.h"

#define KEROS_DEVID_ADDR          0x1c  /* Address including R/W flag */

int keros_init(void);
int keros_authentication(void);
int keros_write_key(uint32_t password, uint8_t page, uint8_t *key,
                    uint8_t encrytion);
int keros_pwchg(uint8_t page, int old_password, int new_password);
#endif /* __KEROS_INTERFACE_H_ */
