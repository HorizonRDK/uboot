
/* SPDX-License-Identifier: GPL-2.0+
 *
 * auto detect pmic and change DTB node
 *
 * Copyright (C) 2020, Horizon Robotics, <yu.kong@horizon.ai>
 */

#include <common.h>
#include <i2c.h>
#include <hb_info.h>

#define DEFAULT_PMIC_ADDR 0xFF

static int do_auto_detect_pmic(cmd_tbl_t *cmdtp, int flag,
		int argc, char *const argv[])
{
    int ret;
    char cmd[128];
    ulong axp1506_addr;
    struct udevice *dev;
    char *pathp  = "/soc/i2c@0xA5009000/axp15060@37";
    char *cnn0_pathp = "/soc/cnn@0xA3000000";
    char *cnn1_pathp = "/soc/cnn@0xA3001000";
    char *cnn0_pd_pathp = "/cnn0_pd_reg_dc@2";
    char *cnn1_pd_pathp = "/cnn1_pd_reg_dc@3";

    if (hb_base_board_type_get() != BASE_BOARD_X3_DVB)
         return 0;

    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
    run_command(cmd, 0);

    /* read ax1506 i2c address from dtb */
    snprintf(cmd, sizeof(cmd), "fdt get value axp1506_addr %s reg", pathp);
    ret = run_command(cmd, 0);
    if (ret)
        return 0;

    /* get axp1506_addr from environment variable */
    axp1506_addr = env_get_ulong("axp1506_addr", 16, DEFAULT_PMIC_ADDR);

    /* get udevice of axp1506 */
    i2c_get_chip_for_busnum(0, (int)axp1506_addr, 1, &dev);

    /* try to read register of axp1506 */
    if (dm_i2c_reg_read(dev, 0x0) < 0) {
        /*
         * detect axp1506 failed, switch to dc-dc regulator
         */
        printf("PMIC invalid, Disable it\n");
        /* diable axp1506 if read register failed */
        snprintf(cmd, sizeof(cmd), "fdt set %s status disabled", pathp);
        run_command(cmd, 0);

        /* get dc-dc regulator phandle value */
        snprintf(cmd, sizeof(cmd),
                "fdt get value cnn0_pd_phandle %s cnn-supply-dc", cnn0_pathp);
        ret = run_command(cmd, 0);
        if (!ret) {
            /* modify cnn-supply to dc-dc regualtor */
            snprintf(cmd, sizeof(cmd),
                    "fdt set %s cnn-supply <${cnn0_pd_phandle}>", cnn0_pathp);
            run_command(cmd, 0);

            /* enable dc-dc regulator dts */
            snprintf(cmd, sizeof(cmd), "fdt set %s status okay", cnn0_pd_pathp);
            run_command(cmd, 0);
        }

        /* get dc-dc regulator phandle value */
        snprintf(cmd, sizeof(cmd),
                "fdt get value cnn1_pd_phandle %s cnn-supply-dc", cnn1_pathp);
        ret = run_command(cmd, 0);
        if (!ret) {
            /* modify cnn-supply to dc-dc regualtor */
            snprintf(cmd, sizeof(cmd),
                    "fdt set %s cnn-supply <${cnn1_pd_phandle}>", cnn1_pathp);
            run_command(cmd, 0);

            /* enable dc-dc regulator dts */
            snprintf(cmd, sizeof(cmd), "fdt set %s status okay", cnn1_pd_pathp);
            run_command(cmd, 0);
        }
        return 0;
    }
    return 0;
}

U_BOOT_CMD(
		detect_pmic, CONFIG_SYS_MAXARGS, 0, do_auto_detect_pmic,
		"detect_pmic",
		"auto detect pmic and change DTB of pmic");
