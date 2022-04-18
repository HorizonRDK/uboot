// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Horizon Robotics.
 */
/*
* Quickboot modify Linux kernel device-tree node
* Rmove node and amend property
*/
#include <common.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <hb_info.h>
#include <linux/libfdt.h>

#ifdef CONFIG_MMC_TUNING_DATA_TRANS
DECLARE_GLOBAL_DATA_PTR;
#define DTS_DWMMC0_PATH "/soc/dwmmc@A5010000"   /* XJ3 mmc storage node */
#endif /*CONFIG_MMC_TUNING_DATA_TRANS*/

/* List of nodes in the kernel device-tree nodes that you want to remove */
char *kernel_dts_node[] = {
    "/soc/dwmmc@A5011000",
    "/soc/dwmmc@A5012000",
    "/soc/nand",
    "/soc/nor",
    NULL};

void hb_quickboot_modify_dts(void)
{
	char cmd[128] = {0};
    char **ptr;

    DEBUG_LOG("Quickboot: clipping dts nodes.\n");
    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
	run_command(cmd, 0);

    for (ptr = kernel_dts_node; *ptr != NULL; ptr++) {
        snprintf(cmd, sizeof(cmd), "fdt rm %s", *ptr);
        run_command(cmd, 0);
    }

	/* write mmc tuning result to Linux kernel dts */
#ifdef CONFIG_MMC_TUNING_DATA_TRANS
	snprintf(cmd, sizeof(cmd), "fdt set %s uboot-tuning-middle-phase %d",
			 DTS_DWMMC0_PATH, gd->mmc_tuning_res);
	run_command(cmd, 0);
#endif /*CONFIG_MMC_TUNING_DATA_TRANS*/
}
