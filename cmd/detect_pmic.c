
/* SPDX-License-Identifier: GPL-2.0+
 *
 * auto detect pmic and change DTB node
 *
 * Copyright (C) 2020, Horizon Robotics, <yu.kong@horizon.ai>
 */

#include <common.h>
#include <i2c.h>
#include <hb_info.h>
#include <linux/libfdt.h>

#define DEFAULT_PMIC_ADDR 0xFF
#define DTB_PATH_MAX_LEN (50)

extern struct fdt_header *working_fdt;
extern int hb_get_cpu_num(void);

static int switch_to_dc(const char *pathp, const char *set_prop,
                const char *get_prop)
{
    int ret = 0;
    int len = 0;
    u64 value = 0;
    int nodeoffset;
    const void *nodep;
    char cmd[128];

    /* get regulator-max-microvolt of cnn0_pd_reg_dc@2/3 */
    nodeoffset = fdt_path_offset(working_fdt, pathp);
    if (nodeoffset < 0) {
        printf("get nodeoffset of %s failed\n", pathp);
        return 1;
    }

    nodep = fdt_getprop(working_fdt, nodeoffset, get_prop, &len);
    if (len <= 0) {
        printf("get nodep of %s %s failed\n", pathp, get_prop);
        return 1;
    }
    value = fdt32_to_cpu(*(fdt32_t *)nodep);
    ret = env_set_ulong("_switch_dc_tmp", value);
    if (ret) {
        printf("set env of %s %s failed\n", pathp, get_prop);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "fdt set %s %s <${_switch_dc_tmp}>",
            pathp, set_prop);
    run_command(cmd, 0);
    return 0;
}
static int do_auto_detect_pmic(cmd_tbl_t *cmdtp, int flag,
		int argc, char *const argv[])
{
    int ret, i = 0;
    char cmd[128];
    ulong axp1506_addr;
    struct udevice *dev;
    char *pathp  = "/soc/i2c@0xA5009000/axp15060@37";
    char cpu_path[][DTB_PATH_MAX_LEN] =
        {"/cpus/cpu@0", "/cpus/cpu@1", "/cpus/cpu@2", "/cpus/cpu@3"};
    char cnn_path[][DTB_PATH_MAX_LEN] =
        {"/soc/cnn@0xA3000000", "/soc/cnn@0xA3001000"};
    char cpu_regu_path[][DTB_PATH_MAX_LEN] =
        {"/cpu_pd_reg_dc"};
    char cnn_regu_path[][DTB_PATH_MAX_LEN] =
        {"/cnn0_pd_reg_dc@2", "/cnn1_pd_reg_dc@3"};
    char *usb_path = "/soc/usb@0xB2000000/";
    char *usb_prop = "usb_0v8-supply";

    if (hb_som_type_get() != SOM_TYPE_X3)
         return 0;

    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
    run_command(cmd, 0);

    /* read ax1506 i2c address from dtb */
    snprintf(cmd, sizeof(cmd), "fdt get value axp1506_addr %s reg", pathp);
    ret = run_command(cmd, 0);
    if (ret) {
        printf("fdt get value axp1506_addr failed\n");
        return 1;
    }

    /* get axp1506_addr from environment variable */
    axp1506_addr = env_get_ulong("axp1506_addr", 16, DEFAULT_PMIC_ADDR);

    /* get udevice of axp1506 */
    i2c_get_chip_for_busnum(0, (int)axp1506_addr, 1, &dev);

    /* try to read register of axp1506 */
    if (dm_i2c_reg_read(dev, 0x0) >= 0) {
	/*x3e cnn opptable is cnn_opp_table_lite*/
	if (hb_get_cpu_num()) {
		printf("change cnn opp table to lite version\n");
		for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
			switch_to_dc(cnn_path[i], "operating-points-v2",
					"operating-points-v2-lite");
		}
	}

        return 0;
    }

    /*
     * detect axp1506 failed, switch to dc-dc regulator
     */
    printf("PMIC invalid, Disable it and delete usb_0v8-supply node\n");
    /* diable axp1506 if read register failed */
    snprintf(cmd, sizeof(cmd), "fdt set %s status disabled", pathp);
    run_command(cmd, 0);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "fdt rm %s %s", usb_path, usb_prop);
    run_command(cmd, 0);
    /* enable dc-dc regulator dts */
    for (i = 0; i < ARRAY_SIZE(cpu_regu_path); ++i) {
        snprintf(cmd, sizeof(cmd), "fdt set %s status okay",
                cpu_regu_path[i]);
        run_command(cmd, 0);
    }
    for (i = 0; i < ARRAY_SIZE(cnn_regu_path); ++i) {
        snprintf(cmd, sizeof(cmd), "fdt set %s status okay",
                cnn_regu_path[i]);
        run_command(cmd, 0);
    }

    /* enable dc-dc regulator dts */
    for (i = 0; i < ARRAY_SIZE(cpu_path); ++i) {
        switch_to_dc(cpu_path[i], "cpu-supply", "cpu-supply-dc");
        switch_to_dc(cpu_path[i], "operating-points-v2",
                    "operating-points-v2-dc");
    }
    for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
        switch_to_dc(cnn_path[i], "cnn-supply", "cnn-supply-dc");
	/*x3e cnn opptable is cnn_opp_table_lite*/
	if (hb_get_cpu_num()) {
		switch_to_dc(cnn_path[i], "operating-points-v2",
			"operating-points-v2-dc-lite");
	} else {
		switch_to_dc(cnn_path[i], "operating-points-v2",
			"operating-points-v2-dc");
	}
    }

    return 0;
}

U_BOOT_CMD(
		detect_pmic, CONFIG_SYS_MAXARGS, 0, do_auto_detect_pmic,
		"detect_pmic",
		"auto detect pmic and change DTB of pmic");
