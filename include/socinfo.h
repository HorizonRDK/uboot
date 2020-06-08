/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
*/
#ifndef SOCINFO_H
#define SOCINFO_H

#include <linux/libfdt.h>
#include <fdt_support.h>

static struct fdt_header *hb_dtb;

int set_boardid(void);

#endif
