// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>
#include <asm/io.h>

extern int boot_stage_mark(int stage);


/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

#ifdef X2_AUTOBOOT
void wait_start(void)
{
    uint32_t reg_val = 0;
    uint32_t boot_flag = 0xFFFFFFFF;
    uint32_t x2_ip = 0;
    char x2_ip_str[32] = {0};

    /* nfs boot argument */
    uint32_t nfs_server_ip = 0;
    char nfs_server_ip_str[32] = {0};
    char nfs_args[256] = {0};

    /* nc:netconsole argument */
    uint32_t nc_server_ip = 0;
    uint32_t nc_mac_high = 0;
    uint32_t nc_mac_low = 0;
    char nc_server_ip_str[32] = {0};
    char nc_server_mac_str[32] = {0};

    char nc_args[256] = {0};


    char bootargs_str[512] = {0};

    while (1) {
        reg_val = readl(BIF_SHARE_REG_OFF);
        if (reg_val == BOOT_WAIT_VAL)
                break;
        printf("wait for client send start cmd: 0xaa\n");
        mdelay(1000);
    }

    /* nfs boot server argument parse */
    boot_flag = readl(BIF_SHARE_REG(5));
    if ((boot_flag&0xFF)==BOOT_VIA_NFS) {
        x2_ip = readl(BIF_SHARE_REG(6));
        sprintf(x2_ip_str, "%d.%d.%d.%d",
        (x2_ip>>24)&0xFF, (x2_ip>>16)&0xFF, (x2_ip>>8)&0xFF, (x2_ip)&0xFF);

        nfs_server_ip = readl(BIF_SHARE_REG(7));
        sprintf(nfs_server_ip_str, "%d.%d.%d.%d",
        (nfs_server_ip>>24)&0xFF, (nfs_server_ip>>16)&0xFF, (nfs_server_ip>>8)&0xFF, (nfs_server_ip)&0xFF);

        sprintf(nfs_args, "root=/dev/nfs nfsroot=%s:/nfs_boot_server,nolock,nfsvers=3 rw "\
                            "ip=%s:%s:192.168.1.1:255.255.255.0::eth0:off rdinit=/linuxrc ",
                            nfs_server_ip_str, x2_ip_str, nfs_server_ip_str);
    }

    /* netconsole server argument parse */
    if ((boot_flag>>8&0xFF)==NETCONSOLE_CONFIG_VALID) {
        x2_ip = readl(BIF_SHARE_REG(6));
        sprintf(x2_ip_str, "%d.%d.%d.%d",
        (x2_ip>>24)&0xFF, (x2_ip>>16)&0xFF, (x2_ip>>8)&0xFF, (x2_ip)&0xFF);

        nc_server_ip = readl(BIF_SHARE_REG(9));
        sprintf(nc_server_ip_str, "%d.%d.%d.%d",
        (nc_server_ip>>24)&0xFF, (nc_server_ip>>16)&0xFF, (nc_server_ip>>8)&0xFF, (nc_server_ip)&0xFF);

        nc_mac_high = readl(BIF_SHARE_REG(10));
        nc_mac_low = readl(BIF_SHARE_REG(11));
        sprintf(nc_server_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
        (nc_mac_high>>8)&0xFF, (nc_mac_high)&0xFF, (nc_mac_low>>24)&0xFF,\
        (nc_mac_low>>16)&0xFF, (nc_mac_low>>8)&0xFF, (nc_mac_low)&0xFF);

        sprintf(nc_args, "netconsole=6666@%s/eth0,9353@%s/%s ", x2_ip_str, nc_server_ip_str, nc_server_mac_str);
    }

    sprintf(bootargs_str, "earlycon clk_ignore_unused %s %s", nfs_args, nc_args);
    env_set("bootargs", bootargs_str);
    return;
}
#endif

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
#ifdef X2_AUTOBOOT
        boot_stage_mark(2);
        wait_start();
#endif
	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifdef CONFIG_VERSION_VARIABLE
	env_set("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();

	run_preboot_environment_command();

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL, NULL, NULL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
