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
//extern int w1_ds28e1x_verifymac(struct w1_slave *sl ,char*secret_buf,unsigned int cvalue);
void w1_ds28e1x_get_rom_id(char* rom_id);

#endif /* _LINUX_W1_GPIO_H */
