
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
#include <configs/xj3.h>

#define DEFAULT_PMIC_ADDR 0xFF
#define DTB_PATH_MAX_LEN (50)

extern struct fdt_header *working_fdt;
extern int hb_get_cpu_num(void);


static void rm_pmic(void)
{
    char *pathp  = DTS_POWER_MANAGEMENT_PATH;
    char cmd[128] = { 0 };

    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
    run_command(cmd, 0);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "fdt rm %s %s", pathp, "pmic");
    run_command(cmd, 0);
}

static void add_pmic(void)
{
    char *pathp  = DTS_POWER_MANAGEMENT_PATH;
    char cmd[128] = { 0 };

    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
    run_command(cmd, 0);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "fdt resize");
    run_command(cmd, 0);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "fdt set %s %s", pathp, "pmic");
    run_command(cmd, 0);
}

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
    if (len <= 0 || nodep == NULL) {
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

static int axp15060_detect() {
    int ret, i = 0;
    char cmd[128];
    ulong axp1506_addr;
    struct udevice *dev_axp15060;
    char *pathp_axp15060  = "/soc/i2c@0xA5009000/axp15060@37";

    /* check axp15060 available */
    /* read ax1506 i2c address from dtb */
    snprintf(cmd, sizeof(cmd), "fdt get value axp1506_addr %s reg", pathp_axp15060);
    ret = run_command(cmd, 0);
    if (ret) {
        printf("fdt get value axp1506_addr failed\n");
    }

    /* get axp1506_addr from environment variable */
    axp1506_addr = env_get_ulong("axp1506_addr", 16, DEFAULT_PMIC_ADDR);

    /* get udevice of axp1506 */
    i2c_get_chip_for_busnum(0, (int)axp1506_addr, 1, &dev_axp15060);

    /* try to read register of axp1506 */
    ret = dm_i2c_reg_read(dev_axp15060, 0x0);
    if ( ret >= 0) {
      printf("axp15060 is valid, enable it\n");
      snprintf(cmd, sizeof(cmd), "fdt set %s status okay", pathp_axp15060);
      run_command(cmd, 0);
    } else {
      printf("axp15060 is invalid, disable it\n");
      /* diable axp1506 if read register failed */
      snprintf(cmd, sizeof(cmd), "fdt set %s status disabled", pathp_axp15060);
      run_command(cmd, 0);
    }
    return ret;
}

static int hpu3501_detect(char *node_path, int i2c_bus)
{
    int ret, i = 0;
    char cmd[128];
    ulong hpu3501_addr;
    struct udevice *dev_hpu3501 = NULL;

    /* check hpu3501 available */
    /* read hpu3501 i2c address from dtb */
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "fdt get value hpu3501_addr %s reg", node_path);
    ret = run_command(cmd, 0);
    if (ret) {
        printf("fdt get value hpu3501_addr failed\n");
    }

    /* get hpu3501_addr from environment variable */
    hpu3501_addr = env_get_ulong("hpu3501_addr", 16, DEFAULT_PMIC_ADDR);

    /* get udevice of hpu3501 */
    i2c_get_chip_for_busnum(i2c_bus, (int)hpu3501_addr, 1, &dev_hpu3501);

    /* try to read register of hpu3501 */
    ret = dm_i2c_reg_read(dev_hpu3501, 0x0);
    if ( ret >= 0) {
      printf("hpu3501 is valid at i2c bus(%d), enable it\n", i2c_bus);
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, sizeof(cmd), "fdt set %s status okay", node_path);
      run_command(cmd, 0);
    } else {
      printf("hpu3501 is invalid at i2c bus(%d), disable it\n", i2c_bus);
      /* diable hpu3501 if read register failed */
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, sizeof(cmd), "fdt set %s status disabled", node_path);
      run_command(cmd, 0);
    }
    return ret;
}

static int switch_to_hpu3501(void) {
  int i;
  char cmd[128];

  char cnn_path[][DTB_PATH_MAX_LEN] =
    {"/soc/cnn@0xA3000000", "/soc/cnn@0xA3001000"};
  for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
    switch_to_dc(cnn_path[i], "cnn-supply", "cnn-supply-hobot");
  }

  char cpu_path[][DTB_PATH_MAX_LEN] =
        {"/cpus/cpu@0", "/cpus/cpu@1", "/cpus/cpu@2", "/cpus/cpu@3"};
  for (i = 0; i < ARRAY_SIZE(cpu_path); ++i) {
    switch_to_dc(cpu_path[i], "cpu-supply", "cpu-supply-hobot");
    switch_to_dc(cpu_path[i], "operating-points-v2",
      "operating-points-v2-hobot");
  }

  char *usb_path = "/soc/usb@0xB2000000/";
  char *usb_prop = "usb_0v8-supply";
  switch_to_dc(usb_path, "usb_0v8-supply", "usb_0v8-supply_hobot");

  add_pmic();
}

static int do_auto_detect_pmic(cmd_tbl_t *cmdtp, int flag,
		int argc, char *const argv[])
{
    int ret, i = 0;
    char cmd[128];
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

    snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
    run_command(cmd, 0);
    if (hb_efuse_chip_type() == CHIP_TYPE_J3) {
        if (is_bpu_clock_limit() == 1) {
            printf("change cnn opp table to lite version\n");
            for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
                switch_to_dc(cnn_path[i], "operating-points-v2",
                        "operating-points-v2-lite");
            }
        }
        return 0;
    }

    if (hpu3501_detect("/soc/i2c@0xA5009000/hpu3501@1e", 0) >= 0
        || hpu3501_detect("/soc/i2c@0xA500A000/hpu3501@1e", 1) >= 0) {
        if (hb_get_cpu_num()) {
            printf("change cnn opp table to lite version\n");
            for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
              switch_to_dc(cnn_path[i], "operating-points-v2",
                  "operating-points-v2-lite");
            }
        }

        switch_to_hpu3501();
        return 0;
    } else if (axp15060_detect() >= 0) {
        // switch to axp15060
        if (hb_get_cpu_num()) {
            printf("change cnn opp table to lite version\n");
            for (i = 0; i < ARRAY_SIZE(cnn_path); ++i) {
              switch_to_dc(cnn_path[i], "operating-points-v2",
                  "operating-points-v2-lite");
            }
        }
        add_pmic();
        return 0;
    } else {
        // switch to dc-dc
    }

    /* for X3DVB only  */
    if (hb_som_type_get() != SOM_TYPE_X3)
        return 0;

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
	if (is_bpu_clock_limit() == 1) {
		switch_to_dc(cnn_path[i], "operating-points-v2",
			"operating-points-v2-dc-lite");
	} else {
		switch_to_dc(cnn_path[i], "operating-points-v2",
			"operating-points-v2-dc");
	}
    }

    /* Removing pmic label，If there is no PMIC on SOM . */
    rm_pmic();

    return 0;
}

U_BOOT_CMD(
		detect_pmic, CONFIG_SYS_MAXARGS, 0, do_auto_detect_pmic,
		"detect_pmic",
		"auto detect pmic and change DTB of pmic");
