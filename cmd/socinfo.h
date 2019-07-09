#ifndef SOCINFO_H
#define SOCINFO_H

#include <linux/libfdt.h>
#include <fdt_support.h>

static struct fdt_header *x2_dtb;

int set_boardid(void);

#endif
