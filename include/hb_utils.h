/*
 * COPYRIGHT NOTICE
 * Copyright 2023 Horizon Robotics, Inc.
 * All rights reserved.
 */

#ifndef __HB_UTILS_H__
#define __HB_UTILS_H__

int hb_ext4_load(char *filename, unsigned long addr, loff_t *len_read);
int hb_getline(char str[], int lim, char **mem_ptr);
int atoi(const char * str);

#endif /* __HB_UTILS_H__ */
