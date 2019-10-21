/*
 * w1-gpio interface to platform code
 *
 * Copyright (C) 2007 Ville Syrjala <syrjala@sci.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#ifndef _LINUX_W1_DS28E1X_H
#define _LINUX_W1_DS28E1X_H

#include "w1/w1.h"
#include "w1/w1_ds28e1x_sha256.h"

void w1_ds28e1x_get_rom_id(char* rom_id);
int w1_ds28e1x_verifymac(struct w1_slave *sl ,char*secret_buf,unsigned int cvalue);
int w1_ds28e1x_write_read_key(struct w1_slave *sl, char *secret_buf, char *r_buf);
int w1_ds28e1x_read_status(struct w1_slave *sl, int pbyte, uchar *rdbuf);
int w1_ds28e1x_write_authblockprotection(struct w1_slave *sl, uchar *data);
int w1_ds28e1x_write_blockprotection(struct w1_slave *sl, uchar block, uchar prot);
int w1_ds28e1x_get_buffer(struct w1_slave *sl, uchar *rdbuf, int retry_limit);
int w1_ds28e1x_write_authblock(struct w1_slave *sl, int page, int segment, uchar *data, int contflag);

#endif /* _LINUX_W1_GPIO_H */
