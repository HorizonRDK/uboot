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
#include <asm/arch/ddr.h>
#include <linux/libfdt.h>
#include <mapmem.h>
#include <veeprom.h>

#include "../arch/arm/cpu/armv8/x2/x2_info.h"

extern int boot_stage_mark(int stage);
extern unsigned int x2_src_boot;


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

static unsigned int x2_board_id[] = {
	0x100, 0x200, 0x201, 0x102, 0x103, 0x101, 0x202, 0x203, 0x301, 0x302, 0x303, 0x304
};

static unsigned int x2_gpio_id[] = {
	0xff, 0xff, 0x30, 0x20, 0x10, 0x00, 0x3C, 0xff, 0x34, 0x36, 0x35, 0x37
};

unsigned int x2_gpio_get(void)
{
	unsigned int reg_x, reg_y;

	reg_x = reg32_read(X2_GPIO_BASE + GPIO_GRP5_REG);
	reg_y = reg32_read(X2_GPIO_BASE + GPIO_GRP4_REG);

	return PIN_BOARD_SEL(reg_x, reg_y);
}

int board_id_verify(unsigned int board_id)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(x2_board_id); i++) {
		if (board_id == x2_board_id[i])
			return 0;
	}

	return -1;
}

unsigned int x2_gpio_to_borad_id(unsigned int gpio_id)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(x2_gpio_id); i++) {
		if (gpio_id == x2_gpio_id[i])
			return x2_board_id[i];
	}

	if (gpio_id == 0x3f)
		return X2_SOM_3V3_ID;

	return 0xff;
}

static unsigned int hex_to_char(unsigned int temp)
{
    uint8_t dst;
    if (temp < 10) {
        dst = temp + '0';
    } else {
        dst = temp -10 +'A';
    }
    return dst;
}

static void uint32_to_char(unsigned int temp, char *s)
{
	int i = 0;
	unsigned char dst;
	unsigned char str[10] = { 0 };

	s[0] = '0';
	s[1] = 'x';

	for (i = 0; i < 4; i++) {
		dst = (temp >> (8 * (3 - i))) & 0xff;

		str[2*i + 2] = (dst >> 4) & 0xf;
		str[2*i + 3] = dst & 0xf;
	}

	for (i =2; i < 10; i++) {
		s[i] = hex_to_char(str[i]);
	}

	s[i + 2] = '\0';
}

static char *x2_bootinfo_dtb_get(unsigned int board_id,
		struct x2_kernel_hdr *config)
{
	char *s = NULL;
	int i = 0;
	char string[20] = { 0 };
	char cmd[256] = { 0 };

	int count = config->dtb_number;
	if (count > 16) {
		printf("error: count %02x not support\n", count);
		return NULL;
	}

	for (i = 0; i < count; i++) {
		if (board_id == config->dtb[i].board_id) {
			s = (char *)config->dtb[i].dtb_name;

			if (x2_src_boot == PIN_2ND_SF) {
				if (config->dtb[i].dtb_addr == 0x0) {
					printf("error: dtb %s not exit\n", s);
					return NULL;
				}

				/* load dtb file */
				uint32_to_char(config->dtb[i].dtb_addr, string);
				strcat(cmd, "sf read 0x4000000 ");
				strcat(cmd, string);

				uint32_to_char(config->dtb[i].dtb_size, string);
				strcat(cmd, " ");
				strcat(cmd, string);
				run_command_list(cmd, -1, 0);

				/* load Image */
				uint32_to_char(config->Image_addr, string);
				memset(cmd, 0, 256);
				strcat(cmd, "sf read 0x80000 ");
				strcat(cmd, string);

				uint32_to_char(config->Image_size, string);
				strcat(cmd, " ");
				strcat(cmd, string);
				run_command_list(cmd, -1, 0);

				/* set bootcmd */
				s = "run ddrboot";
				env_set("bootcmd", s);
			}
			break;
		}
	}

	if (i == count)
		printf("error: board_id %02x not support\n", board_id);

	return s;
}

static void board_dtb_init(void)
{
	int rcode = 0;
	char *s = NULL;
	unsigned int board_id;
	unsigned int gpio_id;
	struct x2_info_hdr *x2_board_type;
	struct x2_kernel_hdr *x2_kernel_conf;

	if (x2_src_boot == PIN_2ND_EMMC) {
		/* load dtb-mapping.conf */
		s = "ext4load mmc 0:4 0x10001000 dtb-mapping.conf\0";
		rcode = run_command_list(s, -1, 0);
		if (rcode != 0) {
			printf("error: dtb-mapping.conf not exit! \n");
			return;
		}
	} else {
		/* load dtb-mapping.conf */
		s = "sf probe\0";
		run_command_list(s, -1, 0);
		s = "sf read 0x10001000 0x194400 0x400\0";
		run_command_list(s, -1, 0);
	}

	x2_board_type = (struct x2_info_hdr *) X2_BOOTINFO_ADDR;
	x2_kernel_conf = (struct x2_kernel_hdr *) X2_DTB_CONFIG_ADDR;
	gpio_id = x2_gpio_get();
	printf("bootinfo/board_id = %02x gpio_id = %02x\n", x2_board_type->board_id, gpio_id);

	board_id = x2_board_type->board_id;

	if (board_id == X2_GPIO_MODE) {

		board_id = x2_gpio_to_borad_id(gpio_id);

		if (board_id == 0xff) {
			printf("error: gpio id %02x not support, set to default x2somfull(101) \n", gpio_id);
			board_id = 0x101;
		} else
			printf("gpio/board_id = %02x\n", board_id);
	} else {
		if (board_id_verify(board_id) != 0) {
			printf("error: board id %02x not support  set to default x2somfull(101) \n", board_id);
			board_id = 0x101;
		}
	}

	if (x2_src_boot == PIN_2ND_EMMC) {
		/* svb board */
		if (board_id == X2_SVB_BOARD_ID || board_id == J2_SVB_BOARD_ID)
			return;
	}

	printf("final/board_id = %02x\n", board_id);
	s = x2_bootinfo_dtb_get(board_id, x2_kernel_conf);

	if (x2_src_boot == PIN_2ND_EMMC) {
		printf("fdtimage = %s\n", s);

		env_set("fdtimage", s);
	}

	return;
}

static int fdt_get_reg(const void *fdt, void *buf, u64 *address, u64 *size)
{
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (address_cells == 2)
		*address = fdt64_to_cpu(*(fdt64_t *)p);
	else
		*address = fdt32_to_cpu(*(fdt32_t *)p);
	p += 4 * address_cells;

	if (size_cells == 2)
		*size = fdt64_to_cpu(*(fdt64_t *)p);
	else
		*size = fdt32_to_cpu(*(fdt32_t *)p);
	p += 4 * size_cells;

	return 0;
}

static int fdt_pack_reg(const void *fdt, void *buf, u64 address, u64 size)
{
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (address_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(address);
	else
		*(fdt32_t *)p = cpu_to_fdt32(address);
	p += 4 * address_cells;

	if (size_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(size);
	else
		*(fdt32_t *)p = cpu_to_fdt32(size);
	p += 4 * size_cells;

	return p - (char *)buf;
}

static int do_change_ion_size(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *path;
	int  nodeoffset;
	char *prop;
	static char data[1024] __aligned(4);
	void *ptmp;
	int  len;
	void *fdt;
	phys_addr_t fdt_paddr;
	u64 ion_start, old_size;
	u32 size;
	char *s = NULL;
	int ret;

	if (argc > 1)
		s = argv[1];

	if (s) {
		size = (u32)simple_strtoul(s, NULL, 10);
		if (size == 0)
			return 0;

		if (size < 64)
			size = 64;
	} else {
		return 0;
	}

	s = env_get("fdt_addr");
	if (s) {
		fdt_paddr = (phys_addr_t)simple_strtoull(s, NULL, 16);
		fdt = map_sysmem(fdt_paddr, 0);
	} else {
		printf("Can't get fdt_addr !!!");
		return 0;
	}

	path = "/reserved-memory/ion_reserved";
	prop = "reg";

	nodeoffset = fdt_path_offset (fdt, path);
	if (nodeoffset < 0) {
		/*
			* Not found or something else bad happened.
			*/
		printf ("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(nodeoffset));
		return 1;
	}
	ptmp = (char *)fdt_getprop(fdt, nodeoffset, prop, &len);
	if (len > 1024) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}
	if (!ptmp)
		return 0;

	fdt_get_reg(fdt, ptmp, &ion_start, &old_size);
	printf("Orign(MAX) Ion Reserve Mem Size to %lldM\n", old_size / 0x100000);

	if (size > old_size / 0x100000) {
		return 0;
	}

	len = fdt_pack_reg(fdt, data, ion_start, size * 0x100000);

	ret = fdt_setprop(fdt, nodeoffset, prop, data, len);
	if (ret < 0) {
		printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}

	printf("Change Ion Reserve Mem Size to %dM\n", size);

	return 0;
}

U_BOOT_CMD(
	ion_modify,	2,	0,	do_change_ion_size,
	"Change ION Reserved Mem Size(Mbyte)",
	"-ion_modify 100"
);

#ifdef X2_AUTORESET
static void prepare_autoreset(void)
{
	printf("prepare for auto reset test ...\n");
	mdelay(50);

	env_set("bootcmd_bak", env_get("bootcmd"));
	env_set("bootcmd", "reset");
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

	if ((x2_src_boot == PIN_2ND_EMMC) || (x2_src_boot == PIN_2ND_SF))
		board_dtb_init();

#ifdef X2_AUTORESET
	prepare_autoreset();
#endif

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
