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
#include <part_efi.h>
#include <mmc.h>
#include <part.h>
#include <memalign.h>
#include <ota.h>
#include <j2-led.h>
#include <configs/x2_config.h>
#include <spi_flash.h>
#include <asm/gpio.h>
#include <ubi_uboot.h>

#include "../arch/arm/cpu/armv8/x2/x2_info.h"

DECLARE_GLOBAL_DATA_PTR;

extern int boot_stage_mark(int stage);
extern unsigned int x2_src_boot;
extern unsigned int sys_sdram_size;
extern struct spi_flash *flash;
bool recovery_sys_enable = true;

int x2j2_get_boardid(void)
{
	int board_id;
	struct x2_info_hdr *x2_board_type;

	x2_board_type = (struct x2_info_hdr *) X2_BOOTINFO_ADDR;
	board_id = x2_board_type ->board_id;
	return board_id;
}
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
	if ((boot_flag&0xFF) == BOOT_VIA_NFS) {
		x2_ip = readl(BIF_SHARE_REG(6));
		snprintf(x2_ip_str, sizeof(x2_ip_str), "%d.%d.%d.%d",
		(x2_ip>>24)&0xFF, (x2_ip>>16)&0xFF, (x2_ip>>8)&0xFF, (x2_ip)&0xFF);

		nfs_server_ip = readl(BIF_SHARE_REG(7));
		snprintf(nfs_server_ip_str, sizeof(nfs_server_ip_str), "%d.%d.%d.%d",
			(nfs_server_ip>>24)&0xFF, (nfs_server_ip>>16)&0xFF,
			(nfs_server_ip>>8)&0xFF, (nfs_server_ip)&0xFF);

		snprintf(nfs_args, sizeof(nfs_args), "root=/dev/nfs "\
				"nfsroot=%s:/nfs_boot_server,nolock,nfsvers=3 rw "\
				"ip=%s:%s:192.168.1.1:255.255.255.0::eth0:off rdinit=/linuxrc ",
				nfs_server_ip_str, x2_ip_str, nfs_server_ip_str);
	}

	/* netconsole server argument parse */
	if ((boot_flag>>8&0xFF) == NETCONSOLE_CONFIG_VALID) {
		x2_ip = readl(BIF_SHARE_REG(6));
		snprintf(x2_ip_str, sizeof(x2_ip_str), "%d.%d.%d.%d",
		(x2_ip>>24)&0xFF, (x2_ip>>16)&0xFF, (x2_ip>>8)&0xFF, (x2_ip)&0xFF);

		nc_server_ip = readl(BIF_SHARE_REG(9));
		snprintf(nc_server_ip_str, sizeof(nc_server_ip_str), "%d.%d.%d.%d",
			(nc_server_ip>>24)&0xFF, (nc_server_ip>>16)&0xFF,
			(nc_server_ip>>8)&0xFF, (nc_server_ip)&0xFF);

		nc_mac_high = readl(BIF_SHARE_REG(10));
		nc_mac_low = readl(BIF_SHARE_REG(11));
		snprintf(nc_server_mac_str, sizeof(nc_server_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
			(nc_mac_high>>8)&0xFF, (nc_mac_high)&0xFF,
			(nc_mac_low>>24)&0xFF, (nc_mac_low>>16)&0xFF,
			(nc_mac_low>>8)&0xFF, (nc_mac_low)&0xFF);

		snprintf(nc_args, sizeof(nc_args), "netconsole=6666@%s/eth0,9353@%s/%s ",
			x2_ip_str, nc_server_ip_str, nc_server_mac_str);
	}

	snprintf(bootargs_str, sizeof(bootargs_str), "earlycon clk_ignore_unused %s %s", nfs_args, nc_args);
	env_set("bootargs", bootargs_str);
	return;
}
#endif

static unsigned int x2_board_id[] = {
	0x100, 0x200, 0x201, 0x102, 0x103, 0x101, 0x202, 0x203, 0x204, 0x205,
	0x300, 0x301, 0x302, 0x303, 0x304, 0x400, 0x401, 0x104, 0x105, 0x106
};

static unsigned int x2_gpio_id[] = {
	0xff, 0xff, 0x30, 0x20, 0x10, 0x00, 0x3C, 0xff, 0xff, 0xff,
	0xff, 0x34, 0x36, 0x35, 0x37, 0xff, 0xff, 0xff, 0xff, 0xff
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

static void nor_dtb_image_load(unsigned int addr, unsigned int leng,
	bool flag)
{
	char cmd[256] = { 0 };
	ulong gz_addr = env_get_ulong("gz_addr", 16, GZ_ADDR);
	ulong dtb_addr = env_get_ulong("fdt_addr", 16, FDT_ADDR);

	if (!flash)
		return;

	if (flag == false) {
		spi_flash_read(flash, addr, leng, (void *)dtb_addr);
	} else {
		spi_flash_read(flash, DTB_MAPPING_ADDR + NOR_SECTOR_SIZE,
			NOR_SECTOR_SIZE * 4, (void *)gz_addr);

		snprintf(cmd, sizeof(cmd), "unzip 0x%lx 0x%lx", gz_addr, dtb_addr);
		run_command_list(cmd, -1, 0);

		gz_addr = dtb_addr + addr;
		if (gz_addr != dtb_addr)
			memcpy((void *)dtb_addr, (void *)gz_addr, NOR_SECTOR_SIZE);
	}
}

#ifdef CONFIG_X2_NAND_BOOT
static void nand_dtb_image_load(unsigned int addr, unsigned int leng,
	bool flag)
{
	char cmd[256] = { 0 };
	ulong gz_addr = env_get_ulong("gz_addr", 16, GZ_ADDR);
	ulong dtb_addr = env_get_ulong("fdt_addr", 16, FDT_ADDR);

	if (flag == false) {
		ubi_volume_read("dtb", (void *)dtb_addr, leng);
	} else {
		ubi_volume_read("dtb", (void *)gz_addr, 0);

		snprintf(cmd, sizeof(cmd), "unzip 0x%lx 0x%lx", gz_addr, dtb_addr);
		run_command_list(cmd, -1, 0);
	}
}

static void x2_nand_env_init(void)
{
	char *tmp;

	/* set bootargs */
	tmp = "earlycon loglevel=8 console=ttyS0 clk_ignore_unused "\
		"mtdids:spi-nand0=hr_nand "\
		"mtdparts=hr_nand:3145728@0x0(bootloader)," \
		"10485760@0x300000(sys),-@0xD00000(rootfs) "\
		"root=ubi0:rootfs ubi.mtd=2,2048 rootfstype=ubifs rw rootwait";
	env_set("bootargs", tmp);

	/* set bootcmd */
	tmp = "send_id;run unzipimage;ion_modify ${ion_size};"\
		"mem_modify ${mem_size};run ddrboot;";
	env_set("bootcmd", tmp);
}

static void x2_nand_dtb_load(unsigned int board_id,
		struct x2_kernel_hdr *config)
{
	char *s = NULL;
	bool gz_flag = false;
	int i, count = config->dtb_number;

	if (count > DTB_MAX_NUM) {
		printf("error: count %02x not support\n", count);
		return NULL;
	}

	if (config->dtb[0].dtb_addr == 0)
		gz_flag = true;

	for (i = 0; i < count; i++) {
		if (board_id == config->dtb[i].board_id) {
			s = (char *)config->dtb[i].dtb_name;

			if ((config->dtb[i].dtb_addr == 0x0) && (gz_flag == false)) {
				printf("error: dtb %s not exit\n", s);
				return NULL;
			}

			nand_dtb_image_load(config->dtb[i].dtb_addr,
				config->dtb[i].dtb_size, gz_flag);

			x2_nand_env_init();
			break;
		}
	}

	if (i == count)
		printf("error: board_id %02x not support\n", board_id);

	printf("fdtimage = %s\n", s);
	env_set("fdtimage", s);
}
#endif

static char *x2_mmc_dtb_load(unsigned int board_id,
		struct x2_kernel_hdr *config)
{
	char *s = NULL;
	int i = 0, count = config->dtb_number;

	if (count > DTB_MAX_NUM) {
		printf("error: count %02x not support\n", count);
		return NULL;
	}

	for (i = 0; i < count; i++) {
		if (board_id == config->dtb[i].board_id) {
			s = (char *)config->dtb[i].dtb_name;
			break;
		}
	}

	if (i == count)
		printf("error: board_id %02x not support\n", board_id);

	return s;
}

static void x2_bootargs_init(unsigned int rootfs_id)
{
	char *s;
	unsigned int len = 0, i;

	/* init bootargs */
	s = env_get("bootargs");
	len = strlen(s);

	for(i = 0; i < len; ++i) {
		if (s[i]  == 'r') {
			if (s[i + 1] == 'o' && s[i + 2] == 'o' && s[i + 3] == 't'
				&& s[i + 4] == '=')
				break;
		}
	}

	if (i < len) {
		s[i + 18] = hex_to_char(rootfs_id);
		env_set("bootargs", s);
	}
}

 static void x2_mmc_env_init(void)
{
	char mmcload[256] = "mmc rescan;ext4load mmc 0:3 ${gz_addr} ${bootfile};";
	char tmp[64] = "ext4load mmc 0:3 ${fdt_addr} ${fdtimage};";
	char logo[64] = "ext4load mmc 0:3 ${logo_addr} logo.rgb";
	char logo_addr[16];

	char rootfs[32] = "system";
	char kernel[32] = "kernel";
	char upmode[16] = { 0 };
	unsigned int rootfs_id, kernel_id;
	char boot_reason[64] = { 0 };
	char *s;
	char count;

	snprintf(logo_addr, sizeof(logo_addr), "0x%x", X2_USABLE_RAM_TOP);
	env_set("logo_addr", logo_addr);
	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
		VEEPROM_RESET_REASON_SIZE);

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	/* get rootfs and kernel partition id */
	rootfs_id = get_partition_id(rootfs);
	kernel_id = get_partition_id(kernel);

	if ((strcmp(upmode, "AB") == 0) || (strcmp(upmode, "golden") == 0)) {
		if (strcmp(boot_reason, "normal") == 0) {
			/* boot_reason is 'normal', normal boot */
			printf("uboot: normal boot \n");

			veeprom_read(VEEPROM_COUNT_OFFSET, &count,
				VEEPROM_COUNT_SIZE);

			if (count <= 0) {
				if (strcmp(upmode, "AB") == 0) {
					/* AB mode, boot system backup partition */
					ota_ab_boot_bak_partition(&rootfs_id, &kernel_id);
				} else {
					/* golden mode, boot recovery system */
					ota_recovery_mode_set(true);
				}
			} else {
				/* get rootfs and kernel partition id */
				rootfs_id = ota_uboot_update_check(rootfs);
				kernel_id = ota_uboot_update_check(kernel);
			}
		} else if (strcmp(boot_reason, "recovery") == 0) {
			/* boot_reason is 'recovery', enter recovery mode */
			env_set("bootdelay", "0");
			ota_recovery_mode_set(false);
		} else {
			/* check update_success flag */
			ota_check_update_success_flag();

			/* get rootfs and kernel partition id */
			rootfs_id = ota_uboot_update_check(rootfs);
			kernel_id = ota_uboot_update_check(kernel);

			/* auto extend last emmc partition */
			if (strcmp(boot_reason, "all") == 0) {
				s = "gpt extend mmc 0";
				run_command_list(s, -1, 0);
			}
		}
	}

	/* init bootargs */
	x2_bootargs_init(rootfs_id);

	/* init env mmcload */
	mmcload[26] = hex_to_char(kernel_id);
	tmp[15] = mmcload[26];
	logo[15] = mmcload[26];
	strcat(mmcload, tmp);
	strcat(mmcload, logo);
	env_set("mmcload", mmcload);

	/* int  env mem_size */
	memset(tmp, 0, sizeof(tmp));
	s = env_get("mem_size");
	if (s == NULL) {
		uint32_to_char(sys_sdram_size, tmp);
		env_set("mem_size", tmp);
	}
}

static void x2_nor_env_init(void)
{
	char *tmp;

	/* set bootargs */
	tmp = "earlycon loglevel=8 console=ttyS0 clk_ignore_unused " \
		"root=ubi0:rootfs ubi.mtd=1 rootfstype=ubifs rw rootwait";
		env_set("bootargs", tmp);

	/* set bootcmd */
	tmp = "send_id;run unzipimage;ion_modify ${ion_size};" \
		"mem_modify ${mem_size};run ddrboot;";
		env_set("bootcmd", tmp);
}

static bool x2_nor_ota_upflag_check(void)
{
	char rootfs[32] = "system";
	char kernel[32] = "kernel";
	char upmode[16] = { 0 };
	char boot_reason[64] = { 0 };
	bool load_imggz = true;
	char count;
	unsigned int ret1, ret2;

	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
		VEEPROM_RESET_REASON_SIZE);

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	if ((strcmp(upmode, "golden") == 0)) {
		if (strcmp(boot_reason, "normal") == 0) {
			/* boot_reason is 'normal', normal boot */
			printf("uboot: normal boot \n");

			veeprom_read(VEEPROM_COUNT_OFFSET, &count,
					VEEPROM_COUNT_SIZE);
			if (count <= 0) {
				/* system boot failed, enter recovery mode */
				ota_recovery_mode_set(true);

				if (recovery_sys_enable)
					load_imggz = false;
			}
		} else if (strcmp(boot_reason, "recovery") == 0) {
			/* boot_reason is 'recovery', enter recovery mode */
			env_set("bootdelay", "0");

			load_imggz = false;
		} else {
			/* check update_success flag */
			if (ota_check_update_success_flag()) {
				load_imggz = false;
			} else {
				/* ota partition status check */
				ret1 = ota_uboot_update_check(rootfs);
				ret2 = ota_uboot_update_check(kernel);

				if ((ret1 == 1) || (ret2 == 1))
					load_imggz = false;
			}
		}
	}
	return load_imggz;
}

static void x2_nor_kernel_load(void)
{
	unsigned int kernel_addr, dtb_addr, head_addr;
	struct x2_flash_kernel_hdr *kernel_hdr;
	char *s;
	bool load_imggz = true;
	ulong gz_addr = env_get_ulong("gz_addr", 16, GZ_ADDR);

	if (flash == NULL) {
		printf("Error: flash init failed\n");
		return NULL;
	}

	load_imggz = x2_nor_ota_upflag_check();
	if (load_imggz)
		head_addr = KERNEL_HEAD_ADDR;
	else
		head_addr = RECOVERY_HEAD_ADDR;

	/* load kernel head */
	spi_flash_read(flash, head_addr, sizeof(struct x2_flash_kernel_hdr),
		(void *)X2_DTB_CONFIG_ADDR);
	kernel_hdr = (struct x2_flash_kernel_hdr *) X2_DTB_CONFIG_ADDR;
	kernel_addr = kernel_hdr->Image_addr + head_addr;
	dtb_addr = kernel_hdr->dtb_addr + head_addr;

	/* check magic */
	s = (char *)kernel_hdr->magic;
	if (strcmp(s, "FLASH0") != 0) {
		printf("Error: kernel magic check failed !\n");
		return NULL;
	}

	/* init boorargs and bootcmd */
	x2_nor_env_init();

	/* load dtb file */
	nor_dtb_image_load(dtb_addr, kernel_hdr->dtb_size, false);

	/* load kernel */
	spi_flash_read(flash, kernel_addr, kernel_hdr->Image_size,
		(void *)gz_addr);

	s = (char *)kernel_hdr->dtbname;

	printf("fdtimage = %s\n", s);
	env_set("fdtimage", s);
}

#ifdef CONFIG_X2_NAND_BOOT
static void x2_nand_kernel_load(struct x2_kernel_hdr *config)
{
	ulong gz_addr;
	bool load_imggz = true;
	gz_addr = env_get_ulong("gz_addr", 16, GZ_ADDR);

	/* TODO: Change to use ubi methods */
#ifdef CONFIG_X2_DUAL_BOOT
	char upmode[16] = { 0 };
	char boot_reason[64] = { 0 };
	char rootfs[32] = "system";
	char kernel[32] = "kernel";
	char count;
	unsigned int ret1, ret2;
	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
		VEEPROM_RESET_REASON_SIZE);

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	if ((strcmp(upmode, "golden") == 0)) {
		if (strcmp(boot_reason, "normal") == 0) {
			/* boot_reason is 'normal', normal boot */
			printf("uboot: normal boot \n");

			veeprom_read(VEEPROM_COUNT_OFFSET, &count,
					VEEPROM_COUNT_SIZE);
			if (count <= 0) {
				/* system boot failed, enter recovery mode */
				ota_recovery_mode_set();

				if (recovery_sys_enable)
					load_imggz = false;
			}
		} else if (strcmp(boot_reason, "recovery") == 0) {
			/* boot_reason is 'recovery', enter recovery mode */
			env_set("bootdelay", "0");
			load_imggz = false;
		} else {
			/* check update_success flag */
			if (ota_check_update_success_flag()) {
				load_imggz = false;
			} else {
				/* ota partition status check */
				ret1 = ota_uboot_update_check(rootfs);
				ret2 = ota_uboot_update_check(kernel);
				if ((ret1 == 1) || (ret2 == 1))
					load_imggz = false;
			}
		}
	}
#endif
	if (load_imggz) {
		/* load Image.gz */
		ubi_volume_read("kernel", (void *)gz_addr, 0);
	} else {
		/* load recovery.gz */
		ubi_volume_read("recovery", (void *)gz_addr, 0);
	}
}
#endif

static void x2_dtb_mapping_load(void)
{
	int rcode = 0;
	char partname[] = "kernel";
	char cmd[256] = { 0 };

	if (x2_src_boot == PIN_2ND_EMMC) {
		/* load dtb-mapping.conf */
		snprintf(cmd, sizeof(cmd), "ext4load mmc 0:%d 0x%x dtb-mapping.conf",
		get_partition_id(partname), X2_DTB_CONFIG_ADDR);
		printf("command is %s\n", cmd);
		rcode = run_command_list(cmd, -1, 0);
		if (rcode != 0) {
			/* load recovery dtb-mapping.conf */
			sprintf(cmd, "ext4load mmc 0:%d 0x%x recovery_dtb/dtb-mapping.conf",
				get_partition_id(partname), X2_DTB_CONFIG_ADDR);
			printf("command is %s\n", cmd);
			rcode = run_command_list(cmd, -1, 0);
			if (rcode !=0) {
				printf("error: dtb-mapping.conf not exit! \n");
				return;
			}
		}
	} else {
#ifdef CONFIG_X2_NAND_BOOT
		ubi_volume_read("dtb_mapping", (void *)X2_DTB_CONFIG_ADDR, 0);
#endif
	}
}

static void x2_env_and_boardid_init(void)
{
	unsigned int board_id;
	unsigned int gpio_id;
	struct x2_info_hdr *x2_board_type;
	struct x2_kernel_hdr *x2_kernel_conf;
	char *s = NULL;
	char boot_reason[64] = { 0 };

	/* load config dtb_mapping */
	x2_dtb_mapping_load();

	x2_board_type = (struct x2_info_hdr *) X2_BOOTINFO_ADDR;
	gpio_id = x2_gpio_get();
	printf("bootinfo/board_id = %02x gpio_id = %02x\n",
		x2_board_type->board_id, gpio_id);

	/* board_id check */
	board_id = x2_board_type->board_id;
	if (board_id == X2_GPIO_MODE) {
		board_id = x2_gpio_to_borad_id(gpio_id);

		if (board_id == 0xff) {
			printf("Error: gpio id %02x not support, using x2dev(101)\n",
				gpio_id);
			board_id = 0x101;
		} else
			printf("gpio/board_id = %02x\n", board_id);
	} else {
		if (board_id_verify(board_id) != 0) {
			printf("Error: board id %02x not support, using x2dev(101)\n",
				board_id);
			board_id = 0x101;
		}
	}

	/* init env recovery_sys_enable */
	s = env_get("recovery_system");
	if (strcmp(s, "disable") == 0) {
		recovery_sys_enable = false;

		/* config resetreason */
		veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
		if (strcmp(boot_reason, "recovery") == 0)
			veeprom_write(VEEPROM_RESET_REASON_OFFSET, "normal",
			VEEPROM_RESET_REASON_SIZE);
	}
	printf("final/board_id = %02x\n", board_id);

	/* mmc or nor env init */
	if (x2_src_boot == PIN_2ND_EMMC) {
		x2_kernel_conf =  (struct x2_kernel_hdr *)X2_DTB_CONFIG_ADDR;
		s = x2_mmc_dtb_load(board_id, x2_kernel_conf);
		printf("fdtimage = %s\n", s);
		env_set("fdtimage", s);

		x2_mmc_env_init();
	} else {
#ifdef  CONFIG_X2_NAND_BOOT
		/* load nand kernel and dtb */
		x2_kernel_conf =  (struct x2_kernel_hdr *)X2_DTB_CONFIG_ADDR;
		x2_nand_dtb_load(board_id, x2_kernel_conf);
		x2_nand_kernel_load(x2_kernel_conf);
#else
		/* load nor kernel and dtb */
		x2_nor_kernel_load();
#endif
	}
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

//START4[prj_j2quad]
//#if defined(CONFIG_X2_QUAD_BOARD)
/* GPIO PIN MUX */
#define PIN_MUX_BASE	0xA6003000
#define GPIO1_CFG (PIN_MUX_BASE + 0x10)

#define SWITCH_PORT_RESET_GPIO	22

/*sja1105 needs reset the certain net port when link state change,
or the network won't work properly*/
static int reboot_notify_to_mcu(void)
{
	int ret;
	unsigned int reg_val;

	/*set gpio1[7] GPIO function*/
	reg_val = readl(GPIO1_CFG);
	reg_val |= 0xc000;
	writel(reg_val, GPIO1_CFG);

	ret = gpio_request(SWITCH_PORT_RESET_GPIO, "switch_port_rst_gpio");
	if (ret && ret != -EBUSY) {
		printf("%s: requesting pin %u failed\n", __func__, SWITCH_PORT_RESET_GPIO);
		return -1;
	}

	ret |= gpio_direction_output(SWITCH_PORT_RESET_GPIO, 1);
	udelay(2000);
	ret |= gpio_set_value(SWITCH_PORT_RESET_GPIO, 0);
	mdelay(10);
	ret |= gpio_set_value(SWITCH_PORT_RESET_GPIO, 1);

	gpio_free(SWITCH_PORT_RESET_GPIO);

	return ret;
}
//#endif
//END4[prj_j2quad]

static int do_set_tag_memsize(cmd_tbl_t *cmdtp, int flag,
	int argc, char * const argv[])
{
	u32 size;
	char *s = NULL;

	if (argc > 1)
		s = argv[1];

	if (s) {
		size = (u32)simple_strtoul(s, NULL, 16);
		if ((size == 0) || (size == sys_sdram_size))
			return 0;
	} else {
		return 0;
	}

	printf("uboot mem_size = %02x\n", size);
	printf("gd mem size is %02llx\n", gd->bd->bi_dram[0].size);

	/* set mem size */
	gd->bd->bi_dram[0].size = size;

	return 0;
}

U_BOOT_CMD(
	mem_modify,	2,	0,	do_set_tag_memsize,
	"Change DDR Mem Size(Mbyte)",
	"-mem_modify 0x40000000"
);

static int nand_write_partition(char *partition, int partition_offset,
				int partition_size)
{
	int ret;
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "mtd erase %s", partition);
	ret = run_command(cmd, 0);
	if (ret)
		printf("mtd erase partition %s failed\n", partition);
	snprintf(cmd, sizeof(cmd), "mtd write %s %x 0x0 %x", partition,
				partition_offset, partition_size);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		printf("mtd write partition %s failed\n", partition);
}

static int burn_nand_flash(cmd_tbl_t *cmdtp, int flag,
	int argc, char * const argv[])
{
	int ret;
	u32 img_addr, img_size;
	u32 bl_size, sys_size, rootfs_size;
	u32 sys_offset, rootfs_offset;
	char *s1 = NULL;
	char *s2 = NULL;
	if (argc != 3) {
		printf("image_addr and img_size must be given\n");
		return 1;
	}
	s1 = argv[1];
	s2 = argv[2];
	img_addr = (u32)simple_strtoul(s1, NULL, 16);
	img_size = (u32)simple_strtoul(s2, NULL, 16);
	printf("Reading image of size 0x%x from address: 0x%x\n", img_size, img_addr);
	bl_size = 0x260000;
	sys_size = 0x560000;
	rootfs_size = img_size - bl_size - sys_size;
	sys_offset = img_addr + bl_size;
	rootfs_offset = sys_offset + sys_size;
	ret = 0;
#ifdef CONFIG_X2_NAND_BOOT
	run_command("ubi detach", 0);
#else
	env_set("mtdids", "spi-nand0=hr_nand");
	env_set("mtdparts", "mtdparts=hr_nand:3145728@0x0(bootloader),10485760@0x300000(sys),-@0xD00000(rootfs)");
#endif
	ret = nand_write_partition("bootloader", img_addr, bl_size);
	ret = nand_write_partition("sys", sys_offset, sys_size);
	ret = nand_write_partition("rootfs", rootfs_offset, rootfs_size);
	return ret;
}

U_BOOT_CMD(
	burn_nand,	3,	0,	burn_nand_flash,
	"Burn decompresssed Image at [addr] with [size] in bytes(hex) to nand flash",
	"-burn_nand [addr] [size]"
);

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
#ifdef CONFIG_X2_NAND_BOOT
	env_set("mtdids", "spi-nand0=hr_nand");
	env_set("mtdparts", "mtdparts=hr_nand:3145728@0x0(bootloader),10485760@0x300000(sys),-@0xD00000(rootfs)");
	run_command("mtd list", 0);
	run_command("mtdparts", 0);
	if (ubi_part("sys", NULL)) {
		panic("system ubi image load failed!\n");
	}
#endif
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

#ifndef CONFIG_DOWNLOAD_MODE
	if ((x2_src_boot == PIN_2ND_EMMC) || (x2_src_boot == PIN_2ND_SF))
		x2_env_and_boardid_init();
#endif

#ifdef X2_AUTORESET
	prepare_autoreset();
#endif

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

//START4[prj_j2quad]
//#if defined(CONFIG_X2_QUAD_BOARD)
	if (x2j2_get_boardid()==QUAD_BOARD_ID){
#if defined(CONFIG_J2_LED)
		j2_led_init();
#endif
		reboot_notify_to_mcu();
		printf("=>j2quad<=");
	}
	if ((x2j2_get_boardid() == J2_MM_BOARD_ID) ||
			(x2j2_get_boardid() == J2_MM_S202_BOARD_ID)) {
		env_set("kernel_addr", "0x400000");
	}
//#endif
//END4[prj_j2quad]
	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
