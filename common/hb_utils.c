/*
 * COPYRIGHT NOTICE
 * Copyright 2023 Horizon Robotics, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/string.h>
#include <malloc.h>
#include <common.h>
#include <part.h>
#include <fs.h>

int hb_ext4_load(char *filename, unsigned long addr, loff_t *len_read)
{
      int ret;
      char dev_part_str[16];
      char *devtype, *devnum, *devplist;

      devtype = env_get("devtype");
      if (!devtype) {
         return 1;
      }

      devnum = env_get("devnum");
      if (!devnum) {
         return 1;
      }

      devplist = env_get("devplist");
      if (!devplist) {
         return 1;
      }

      sprintf(dev_part_str, "%s:%s", devnum, devplist);
      if (fs_set_blk_dev(devtype, dev_part_str, FS_TYPE_EXT))
         return 1;
      ret = fs_read(filename, addr, 0, 0, len_read);
      if (ret < 0)
         return 1;

      return 0;
}

int hb_getline(char str[], int lim, char **mem_ptr)
{
    char c = '\0';
    int i;
    for (i = 0; i < lim - 1 && ((c = *(*mem_ptr)) != '\0' && c != '\n'); ++i) {
        str[i] = c;
        (*mem_ptr)++;
    }
    if (c == '\0') {
        str[i] = '\0';
        return -1;
    }
    if (c == '\n') {
        str[i] = c;
        (*mem_ptr)++;
        i++;
    }
    str[i] = '\0';
    return i;
}

#define false 0
#define true 1

char valid=true;

int atoi(const char * str)
{
    char minus=false;
    long long result=0;
    valid=false;
    if(str==NULL)
        return 0;
    while(*str==' ')
        str++;
    if(*str=='-')
    {
        minus=true;
        str++;
    }
    else if(*str=='+')
        str++;
    if(*str<'0'||*str>'9')
        return 0;

    valid=true;
    while(*str>='0' && *str<='9')
    {
        result=result*10+*str-'0';
        if((minus && result>INT_MAX + 1LL) || (!minus && result>INT_MAX))
        {
            valid=false;
            return 0;
        }

        str++;
    }

    if(minus)
        result*=-1;
    return (int)result;
}
