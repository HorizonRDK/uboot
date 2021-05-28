/*
 * (C) Copyright 2019-12-11
 *
 * uboot watchdog support
 *
 */

#include <common.h>
#include <command.h>
#include <watchdog.h>

static int do_watchdog_config(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	char *config = NULL;

	if (argc != 2) {
		printf("error: parameter number %d not support! \n", argc);
		return CMD_RET_USAGE;
	}

	config = argv[1];

	if (strcmp(config, "on") == 0) {
		hb_wdt_init_hw();
		hb_wdt_start(NORMAL_WDT);
		printf("enable watchdog success !\n");
	} else if (strcmp(config, "off") == 0) {
		hb_wdt_stop();
		printf("disable watchdog success !\n");
	} else {
		printf("error: flag %s not support! \n", config);
		return CMD_RET_FAILURE;
	}

	return 0;
}

U_BOOT_CMD(watchdog, 2, 0, do_watchdog_config,
	   "enable or disable watchdog function \n",
	   "<on|off> \n"
	   "    - on \n"
	   "          enable watchdog, default timeout 10s\n"
	   "    - off \n"
	   "          disable watchdog\n"
);
