/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */
#include <common.h>
#include <cli.h>
#include <sata.h>
#include <ahci.h>
#include <scomp.h>
#include <scsi.h>
#include <malloc.h>
#include <asm/io.h>
#include <cpu.h>
#include <veeprom.h>
#include <sysreset.h>
#include <linux/libfdt.h>
#include <mapmem.h>
#include <part_efi.h>
#include <mmc.h>
#include <part.h>
#include <memalign.h>
#include <ota.h>
#include <spi_flash.h>
#include <asm/gpio.h>
#include <asm/arch/hb_reg.h>
#ifdef CONFIG_HB_BIFSD
#include <asm/arch/hb_bifsd.h>
#endif
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_share.h>

#include <hb_info.h>
#include <ubi_uboot.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/boot_mode.h>
#include <asm/arch-x2/ddr.h>
#include <i2c.h>
#include <mtd.h>
#include "../../../cmd/legacy-mtd-utils.h"

#define HB_PIN_FUNC_CFG_REG(p)  (PIN_MUX_BASE + ((p) * 0x4))
#define HB_IO_OUT_CTL_REG(p)    (GPIO_BASE + ((p) / 16) * 0x10 + 0x8)
#define HB_IO_IN_VAL_REG(p)     (GPIO_BASE + ((p) / 16) * 0x10 + 0xc)
#define ARRAY_LEN(a) 			(sizeof(a) / sizeof((a)[0]))

#ifdef CONFIG_HBOT_SECURE_ENGINE
#include <hb_spacc.h>
#include <hb_pka.h>
#endif

#if defined(CONFIG_HB_WATCHDOG) && \
		!(defined(CONFIG_HB_AP_BOOT) || defined(CONFIG_HB_YMODEM_BOOT))
#include <watchdog.h>
#endif
extern struct spi_flash *flash;
#ifndef CONFIG_FPGA_HOBOT
extern unsigned int sys_sdram_size;
extern bool recovery_sys_enable;
#endif
#if defined(HB_SWINFO_DUMP_OFFSET) && !defined(CONFIG_FPGA_HOBOT)
static int stored_dumptype;
#endif

extern unsigned int hb_gpio_get(void);
extern unsigned int hb_gpio_to_board_id(unsigned int gpio_id);
extern unsigned int detect_baud(void);

#ifdef CONFIG_TARGET_XJ3
extern void disable_pll(void);
extern void change_sys_pclk_250M(void);
extern int hb_get_cpu_num(void);
#endif
int32_t hb_som_type = -1;
int32_t hb_base_board_type = -1;
char hb_upmode[32] = UPMODE_GOLDEN;
char hb_bootreason[32] = REASON_NORMAL;
char hb_partstatus = 0;
uint16_t ion_cma_status = 1;
bool custom_bootargs = false;

struct hb_uid_hdr hb_unique_id;

#ifdef CONFIG_SYSRESET
static int print_resetinfo(void)
{
	struct udevice *dev;
	char status[256];
	int ret;

	ret = uclass_first_device_err(UCLASS_SYSRESET, &dev);
	if (ret) {
		debug("%s: No sysreset device found (error: %d)\n",
		      __func__, ret);
		/* Not all boards have sysreset drivers available during early
		 * boot, so don't fail if one can't be found.
		 */
		return 0;
	}

	if (!sysreset_get_status(dev, status, sizeof(status)))
		printf("%s", status);

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO) && CONFIG_IS_ENABLED(CPU)
static int print_cpuinfo(void)
{
	struct udevice *dev;
	char desc[512];
	int ret;

	ret = uclass_first_device_err(UCLASS_CPU, &dev);
	if (ret) {
		debug("%s: Could not get CPU device (err = %d)\n",
		      __func__, ret);
		return ret;
	}

	ret = cpu_get_desc(dev, desc, sizeof(desc));
	if (ret) {
		debug("%s: Could not get CPU description (err = %d)\n",
		      dev->name, ret);
		return ret;
	}

	printf("%s", desc);

	return 0;
}
#endif

#ifdef HB_AUTOBOOT
static int boot_stage_mark(int stage)
{
        int ret = 0;

        switch (stage) {
        case 0:
                writel(BOOT_STAGE0_VAL, BIF_SHARE_REG_OFF);
                break;
        case 1:
                writel(BOOT_STAGE1_VAL, BIF_SHARE_REG_OFF);
                break;
        case 2:
                writel(BOOT_STAGE2_VAL, BIF_SHARE_REG_OFF);
                break;
        case 3:
                writel(BOOT_STAGE3_VAL, BIF_SHARE_REG_OFF);
                break;
        default :
                ret = -EINVAL;
                break;
        }

        return ret;
}
#endif

static int bif_change_reset2gpio(void)
{
	unsigned int reg_val;
	/*set gpio1[15] GPIO function*/
#ifdef CONFIG_TARGET_XJ3
	reg_val = readl(PIN_MUX_BASE + 0x7c);
	reg_val = PIN_CONFIG_GPIO(reg_val);
	writel(reg_val, PIN_MUX_BASE + 0x7c);
#else
	/* XJ2 GPIO REG */
	reg_val = readl(GPIO1_CFG);
	reg_val |= 0xc0000000;
	writel(reg_val, GPIO1_CFG);
#endif
	return 0;
}


#if defined(CONFIG_HB_BIFSD)
static int initr_bifsd(void)
{
	hb_bifsd_initialize();
	return 0;
}
#endif /* CONFIG_HB_BIFSD */
#ifndef CONFIG_FPGA_HOBOT
static int  disable_cnn(void)
{
	u32 reg;
	/* Disable clock of CNN */
	writel(0x33, HB_CNNSYS_CLKEN_CLR);
	while (!((reg = readl(HB_CNNSYS_CLKOFF_STA)) & 0xF));

	udelay(5);

	reg = readl(HB_PMU_VDD_CNN_CTRL) | 0x22;
	writel(reg, HB_PMU_VDD_CNN_CTRL);
	udelay(5);

	writel(0x3, HB_SYSC_CNNSYS_SW_RSTEN);
	udelay(5);

	DEBUG_LOG("Disable cnn cores ..\n");
	return 0;
}
#endif

#ifdef ENABLE_BIFSPI
static int bif_recover_reset_func(void)
{
	unsigned int reg_val;

	/*set gpio1[15] GPIO function*/
	reg_val = readl(GPIO1_CFG);
	reg_val &= ~(0xc0000000);
	writel(reg_val, GPIO1_CFG);
	return 0;
}

#define HB_AP_ROOTFS_TYPE_NONE			1
#define HB_AP_ROOTFS_TYPE_CPIO			2
#define HB_AP_ROOTFS_TYPE_SQUASHFS		3
#define HB_AP_ROOTFS_TYPE_CRAMFS		4

static int apbooting(void)
{
	unsigned int kernel_addr, dtb_addr, initrd_addr, rootfstype;
	char cmd[256] = { 0 };
	char *rootfstype_str;
	char *squashfs_str = "squashfs";
	char *cramfs_str = "cramfs";

	bif_change_reset2gpio();
	if (readl(HB_SHARE_BOOT_KERNEL_CTRL) == 0x5aa5) {
		writel(DDRT_UBOOT_RDY_BIT, HB_SHARE_DDRT_CTRL);
		printf("-- wait for kernel\n");
		while (!(readl(HB_SHARE_DDRT_CTRL) == 0)) {}
		kernel_addr = readl(HB_SHARE_KERNEL_ADDR);
		dtb_addr = readl(HB_SHARE_DTB_ADDR);
		rootfstype = readl(HB_SHARE_ROOTFSTYPE_ADDR);
		initrd_addr = readl(HB_SHARE_INITRD_ADDR);
		bif_recover_reset_func();
		// set bootargs if input rootfs is cramfs or squashfs
		if ((HB_AP_ROOTFS_TYPE_SQUASHFS == rootfstype)
				|| (HB_AP_ROOTFS_TYPE_CRAMFS == rootfstype)) {
			if (HB_AP_ROOTFS_TYPE_SQUASHFS == rootfstype)
				rootfstype_str = squashfs_str;
			else
				rootfstype_str = cramfs_str;
			snprintf(cmd, sizeof(cmd)-1, "earlycon console=ttyS0 clk_ignore_unused "
					"root=/dev/ram0 ro initrd=0x%x,100M rootfstype=%s rootwait",
					initrd_addr, rootfstype_str);
			env_set("bootargs", cmd);
		}
		snprintf(cmd, sizeof(cmd)-1, "booti 0x%x - 0x%x", kernel_addr, dtb_addr);
		printf("cmd: %s\n", cmd);
		run_command_list(cmd, -1, 0);
		// run_command_list("booti 0x80000 - 0x10000000", -1, 0);
	}
	bif_recover_reset_func();
	return 0;
}
#endif

#ifdef CONFIG_AP_CP_COMN_MODE
struct hb_ap_comn_handle{
	unsigned int magic[4];		/* 0xFEFEFEFE */
	unsigned int cmd_status[4]; /* 0: free 1: ready 2: running 3:finish */
	unsigned int cmd_result[4]; /* 0: success  1: failed  */
};

#define AP_CP_COMN_ADDR (HB_BOOTINFO_ADDR)
#define AP_BUF_ADDR (AP_CP_COMN_ADDR + 0x200)
#define CP_BUF_ADDR (AP_CP_COMN_ADDR + 0x300)
#define STATUS_READ 0
#define STATUS_WRITE 1

static void receive_msg(char *buf, unsigned int size)
{
	if (NULL == buf)
		return;

	memcpy(buf, (void *)AP_BUF_ADDR, size);
}

static void send_msg(char *buf, unsigned int size)
{
	if (NULL == buf)
		return;

	memcpy((void *)CP_BUF_ADDR, buf, size);
}

static void hb_handle_status(struct hb_ap_comn_handle *handle,
	unsigned int flag)
{
	if (NULL == handle)
		return;

	/* 0: read  1: write */
	if (flag == 0)
		memcpy(handle, (void *)AP_CP_COMN_ADDR, sizeof(struct hb_ap_comn_handle));
	else
		memcpy((void *)AP_CP_COMN_ADDR, handle, sizeof(struct hb_ap_comn_handle));
}

static int hb_ap_communication(void)
{
	char receive_info[256] = { 0 };
	char send_info[256] = { 0 };
	int ret = 0;
	char cache_off[32] = "dcache off";
	struct hb_ap_comn_handle handle = { 0 };

	/* magic check */
	hb_handle_status(&handle, STATUS_READ);
	if (handle.magic[0] != 0xFEFEFEFE)
		return 0;

	/* close uboot dcache */
	run_command_list(cache_off, -1, 0);

	/* init handle */
	handle.cmd_status[0] = 0;
	handle.cmd_result[0] = 0;

	/* update status to memory */
	hb_handle_status(&handle, STATUS_WRITE);

	printf("***** uboot AP-CP communication mode ! *****\n");
	printf("***** waiting cmd from AP: *****\n");

	while(1) {
		/* read status */
		hb_handle_status(&handle, STATUS_READ);

		if (handle.cmd_status[0] == 0 || handle.cmd_status[0] == 3) {
			mdelay(1000);
		} else {
			/* read cmd from buffer */
			receive_msg(receive_info, sizeof(receive_info));
			snprintf(receive_info, sizeof(receive_info), "%s", receive_info);

			/* update status */
			handle.cmd_status[0] = 2;
			hb_handle_status(&handle, STATUS_WRITE);

			/* run command */
			printf("receive_cmd : %s\n", receive_info);
			printf("run cmd : \n");
			ret = run_command_list(receive_info, -1, 0);
			if (ret != 0) {
				handle.cmd_result[0] = 1;
				snprintf(send_info, sizeof(send_info), "CP run cmd failed! ");
			} else {
				handle.cmd_result[0] = 0;
				snprintf(send_info, sizeof(send_info), "CP run cmd success! ");
			}

			/* send info */
			send_msg(send_info, sizeof(send_info));

			/* update status */
			handle.cmd_status[0] = 3;
			hb_handle_status(&handle, STATUS_WRITE);

			printf("run cmd finish !\n");
			printf("***** waiting cmd from AP: *****\n");
		}
	}

	return 0;
}
#endif

#ifdef HB_AUTOBOOT
static void wait_start(void)
{
    uint32_t reg_val = 0;
    uint32_t boot_flag = 0xFFFFFFFF;
    uint32_t hb_ip = 0;
    char hb_ip_str[32] = {0};

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
		hb_ip = readl(BIF_SHARE_REG(6));
		snprintf(hb_ip_str, sizeof(hb_ip_str), "%d.%d.%d.%d",
		(hb_ip>>24)&0xFF, (hb_ip>>16)&0xFF, (hb_ip>>8)&0xFF, (hb_ip)&0xFF);

		nfs_server_ip = readl(BIF_SHARE_REG(7));
		snprintf(nfs_server_ip_str, sizeof(nfs_server_ip_str), "%d.%d.%d.%d",
			(nfs_server_ip>>24)&0xFF, (nfs_server_ip>>16)&0xFF,
			(nfs_server_ip>>8)&0xFF, (nfs_server_ip)&0xFF);

		snprintf(nfs_args, sizeof(nfs_args), "root=/dev/nfs "\
				"nfsroot=%s:/nfs_boot_server,nolock,nfsvers=3 rw "\
				"ip=%s:%s:192.168.1.1:255.255.255.0::eth0:off rdinit=/linuxrc ",
				nfs_server_ip_str, hb_ip_str, nfs_server_ip_str);
	}

	/* netconsole server argument parse */
	if ((boot_flag>>8&0xFF) == NETCONSOLE_CONFIG_VALID) {
		hb_ip = readl(BIF_SHARE_REG(6));
		snprintf(hb_ip_str, sizeof(hb_ip_str), "%d.%d.%d.%d",
		(hb_ip>>24)&0xFF, (hb_ip>>16)&0xFF, (hb_ip>>8)&0xFF, (hb_ip)&0xFF);

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
			hb_ip_str, nc_server_ip_str, nc_server_mac_str);
	}

	snprintf(bootargs_str, sizeof(bootargs_str), "earlycon clk_ignore_unused %s %s", nfs_args, nc_args);
	env_set("bootargs", bootargs_str);
	return;
}
#endif

static void hb_mipi_panel_reset(void)
{
	uint32_t reg = 0;
	uint32_t base_board_id = hb_base_board_type_get();

	if (base_board_id == BASE_BOARD_X3_DVB) {
		reg = reg32_read(BIFSPI_CLK_PIN_REG);
		reg32_write(BIFSPI_CLK_PIN_REG, PIN_CONFIG_GPIO(reg));
		reg = reg32_read(X2_GPIO_BASE + X3_GPIO1_CTRL_REG);
		reg32_write(X2_GPIO_BASE + X3_GPIO1_CTRL_REG, X3_MIPI_RESET_OUT_LOW(reg));
	}
}
#ifdef CONFIG_DM_I2C
static bool hb_pf5024_device_id_getable(void)
{
	uint8_t chip = I2C_PF5024_SLAVE_ADDR;
	uint32_t addr = 0;
	uint8_t device_id = 0;
	int ret;
	struct udevice *dev;

	ret = i2c_get_cur_bus_chip(chip, &dev);
	if (!ret)
		ret = i2c_set_chip_offset_len(dev, 1);
	else
		return false;

	ret = dm_i2c_read(dev, addr, &device_id, 1);
	if (ret)
		return false;

	if (device_id == 0x54)
		return true;
	else
		return false;
}
#endif

/*
 *              BIFSD_CLK(20) BIFSD_DATA1(23) SD0_DATA2(57) som_type
 * SDBv3      :      0             1              x          3
 * SDBv4      :      0             0              x          4
 * X3 PI_v1.1 :      1             x              1          5
 * X3 PI_v2.0 :      1             x              0          6
 */
static int hb_adjust_somid_by_gpios(void)
{
	int16_t i = 0;
	uint32_t pin_val = 0;
	uint32_t pin_no[] = {23, 20};
	uint64_t addr = 0, reg = 0;
	uint16_t pin_nums = ARRAY_LEN(pin_no);

	/* set pin func to GPIO*/
	/* setting PD */
	addr = HB_PIN_FUNC_CFG_REG(pin_no[0]);
	reg = readl(addr);
	reg |= 0x83;
	reg &= ~(1 << 8);
	writel(reg, addr);

	addr = HB_PIN_FUNC_CFG_REG(pin_no[1]);
	reg = readl(addr);
	reg |= 0x3;
	writel(reg, addr);

	/* set pin to GPIO input function */
	for (i = 0; i < pin_nums; ++i) {
		addr = HB_IO_OUT_CTL_REG(pin_no[i]);
		reg = readl(addr);
		reg &= ~((uint64_t)(0x1) << (16 + pin_no[i] % 16));
		writel(reg, addr);
	}
	udelay(15*1000);
	for (i = 0; i < pin_nums; ++i) {
		addr = HB_IO_IN_VAL_REG(pin_no[i]);
		reg = readl(addr);
		if (reg & ((uint64_t)(0x1) << (pin_no[i] % 16))) {
			pin_val |= 0x1 << i;
		}
	}

	if (pin_val > 2) pin_val = 2;

	switch (pin_val) {
	case 0:
		return SOM_TYPE_X3SDBV4;
	case 1:
		return SOM_TYPE_X3SDB;
	case 2:
	{
		addr = HB_PIN_FUNC_CFG_REG(57);
		reg = readl(addr);
		reg |= 0x83;
		reg &= ~(1 << 8);
		writel(reg, addr);
		addr = HB_IO_OUT_CTL_REG(57);
		reg = readl(addr);
		reg &= ~((uint64_t)(0x1) << (16 + 57 % 16));
		writel(reg, addr);
		udelay(15*1000);
		addr = HB_IO_IN_VAL_REG(57);
		reg = readl(addr);
		pin_val = reg & ((uint64_t)(0x1) << (57 % 16));
		if (pin_val)
			return SOM_TYPE_X3PI;
		else
			return SOM_TYPE_X3PIV2;
	}
	default:
		return SOM_TYPE_X3SDB;
	}
}


uint32_t hb_som_type_get(void)
{
	uint32_t som_id;
#ifdef HR_SOM_TYPE
	return HR_SOM_TYPE;
#endif

	if (hb_som_type < 0) {
		som_id = SOM_TYPE_SEL(hb_board_id);

		switch (som_id) {
		case AUTO_DETECTION:
			som_id = SOM_TYPE_X3;
			break;
		case SOM_TYPE_X3:
		case SOM_TYPE_J3:
			break;
		case SOM_TYPE_X3SDB:
			som_id = hb_adjust_somid_by_gpios();
			break;
		case SOM_TYPE_X3SDBV4:
		case SOM_TYPE_X3PI:
		case SOM_TYPE_X3PIV2:
		case SOM_TYPE_X3E:
		default:
			break;
		}
		hb_som_type = som_id;
		DEBUG_LOG("hb som type: %d\n", hb_som_type);
	} else {
		return hb_som_type;
	}

	return som_id;
}


uint32_t hb_base_board_type_get(void)
{
	uint32_t reg, base_id;

#ifdef HR_BASE_BOARD_TYPE
	return HR_BASE_BOARD_TYPE;
#endif

	if (hb_base_board_type < 0) {
		base_id = BASE_BOARD_SEL(hb_board_id);

		switch (base_id) {
		case AUTO_DETECTION:
			reg = reg32_read(X2_GPIO_BASE + X3_GPIO0_VALUE_REG);
			base_id = PIN_BASE_BOARD_SEL(reg) + 1;
			break;
		case BASE_BOARD_X3_DVB:
		case BASE_BOARD_J3_DVB:
		case BASE_BOARD_CVB:
		case BASE_BOARD_X3_SDB:
			break;
		default:
			break;
		}
		hb_base_board_type = base_id;
	} else {
		return hb_base_board_type;
	}

	return base_id;
}

uint32_t hb_board_type_get(void)
{
	uint32_t board_type, base_board_id, som_id;

	base_board_id = hb_base_board_type_get();
	som_id = hb_som_type_get();

	board_type = (base_board_id & 0x7) | ((som_id & 0xf) << 8);
	DEBUG_LOG("board_type = %02x\n", board_type);

	return board_type;
}

int32_t hb_board_type_get_by_pin(int pin_nums)
{
	int16_t i = 0;
    uint32_t board_type = 0;
    uint32_t pin_no[] = {BOARD_TYPE_PIN_0, BOARD_TYPE_PIN_1, BOARD_TYPE_PIN_2};
    uint64_t addr = 0, reg = 0;

	if (pin_nums > ARRAY_LEN(pin_no)) {
		printf("pin_nums passed is greater than largest pin config!\n");
		return -1;
	}
    /* set Board type pin func to GPIO*/
    for (i = 0; i < pin_nums; ++i) {
        addr = HB_PIN_FUNC_CFG_REG(pin_no[i]);
        reg = readl(addr);
        reg |= 0x3;
        writel(reg, addr);
    }

    /* set Board type pin to GPIO input function */
    for (i = 0; i < pin_nums; ++i) {
        addr = HB_IO_OUT_CTL_REG(pin_no[i]);
        reg = readl(addr);
        reg &= ~((uint64_t)(0x1) << (16 + pin_no[i] % 16));
        writel(reg, addr);
    }
    udelay(500);
    for (i = 0; i < pin_nums; ++i) {
        addr = HB_IO_IN_VAL_REG(pin_no[i]);
        reg = readl(addr);
        if (reg & ((uint64_t)(0x1) << (pin_no[i] % 16))) {
            board_type |= 0x1 << i;
        }
    }
    DEBUG_LOG("board_type_by_pin = %02x\n", board_type);
    return board_type;
}

#ifndef CONFIG_FPGA_HOBOT
static void hb_boot_args_cmd_set(int boot_mode)
{
	char bootargs_str[2048] = { 0 };
	char tmp[256] = { 0 };
	char *extra_bootargs = NULL;
#if (defined(CONFIG_HB_BOOT_FROM_NOR) || defined(CONFIG_HB_BOOT_FROM_NAND)) && !defined(CONFIG_DISTRO_DEFAULTS)
	/* rootfs_mtd_name is used for Flashes, now there is no B partition */
	char *rootfs_mtd_name = "system";
	char *rootfs_vol_name = "rootfs";
	char *boot_mtd_name = "boot";
	int ret = 0, rootfs_mtdnm = -1;
	struct mtd_info *root_mtd, *boot_mtd;
	struct ubi_volume *vol;
#endif /*(defined(CONFIG_HB_BOOT_FROM_NOR) || defined(CONFIG_HB_BOOT_FROM_NAND)) && !defined(CONFIG_DISTRO_DEFAULTS)*/
	int nr_cpus = 0;
	int if_secure = hb_check_secure();
#ifdef CONFIG_TARGET_XJ3
	nr_cpus = hb_get_cpu_num();
#endif
	char ubuntu_magic[4] = { 0 };
	bool ubuntu_boot = false;
	char *ubuntu_bootargs = NULL;
	veeprom_read(VEEPROM_UBUNTU_MAGIC_OFFSET, ubuntu_magic, VEEPROM_UBUNTU_MAGIC_SIZE);
	ubuntu_boot = (strncmp(UBUNTU_MAGIC, ubuntu_magic, sizeof(ubuntu_magic)) == 0);
	printf("hb_boot_args_cmd_set custom_bootargs %d ubuntu_boot %d\n", custom_bootargs, ubuntu_boot);
	/* Set Bootargs */
	if (!custom_bootargs) {
		/* General Bootargs */
		snprintf(bootargs_str, sizeof(bootargs_str),
			"%s,%d raid=noautodetect hobotboot.reson=%s",
			CONFIG_BOOTARGS, detect_baud(), hb_reset_reason_get());
		/* Add nr_cpus */
		if (nr_cpus > 0) {
			snprintf(tmp, sizeof(tmp), " nr_cpus=%d", nr_cpus);
			strncat(bootargs_str, tmp, sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		}
		if (!if_secure || (if_secure && boot_mode != PIN_2ND_EMMC)) {
			/* Add Rootfstype, passed from Macro during build */
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), " rootfstype="__stringify(ROOTFS_TYPE));
			strncat(bootargs_str, tmp,
					sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		}

		/* Specific bootargs for each bootmode */
		memset(tmp, 0, sizeof(tmp));

#if defined CONFIG_HB_BOOT_FROM_MMC
		if(!if_secure) {
			strncat(bootargs_str, " ro" ,
					sizeof(bootargs_str) - strlen(bootargs_str) - 1);
			strncat(bootargs_str, " rootwait",
					sizeof(bootargs_str) - strlen(bootargs_str) - 1);
			snprintf(tmp, sizeof(tmp), " root=/dev/mmcblk0p%d",
					get_partition_id(system_partition));
			strncat(bootargs_str, tmp,
					sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		}
#elif(defined(CONFIG_HB_BOOT_FROM_NOR) || defined(CONFIG_HB_BOOT_FROM_NAND)) && !defined(CONFIG_DISTRO_DEFAULTS)
		/* ro/rw judgement from volume type */
		ubi_part(rootfs_mtd_name, NULL);
		vol = ubi_find_volume(rootfs_vol_name);
		if (vol != NULL) {
			strncat(bootargs_str,
				vol->vol_type == UBI_STATIC_VOLUME ? " ro" : " rw",
				sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		} else {
			printf("Rootfs volume: %s not found in UBI partition: %s\n",
					rootfs_vol_name, rootfs_mtd_name);
			/* Stop at UBoot */
			env_set("bootdelay", "-1");
		}

		strncat(bootargs_str, " rootwait",
				sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		root_mtd = get_mtd_device_nm(rootfs_mtd_name);
		rootfs_mtdnm = (root_mtd == NULL) ? 4 : (root_mtd->index - 1);
		/* If neccessary, add logic to determine which ubi device is
			rootfs, ubi0 and volume name rootfs is used as default */
		snprintf(tmp, sizeof(tmp), " root=ubi0:rootfs ubi.mtd=%d", rootfs_mtdnm);
		strncat(bootargs_str, tmp,
			sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		if (boot_mode == PIN_2ND_NAND) {
			/* For nand, page size must also be specified */
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), ",%d",
						(root_mtd == NULL) ? 2048 : root_mtd->writesize);
			strncat(bootargs_str, tmp,
				sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		}
		/* For Flashes, Kernel MTD partition is constructed with mtdparts */
		strncat(bootargs_str, " ", sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		strncat(bootargs_str, env_get("mtdparts"),
				sizeof(bootargs_str) - strlen(bootargs_str) - 1);
#endif /*defined CONFIG_HB_BOOT_FROM_MMC*/
		if (ubuntu_boot == true)
		{
			char *ptr = strstr(bootargs_str,"rootfstype");
			memset(ptr, 0, strlen(ptr));
			strcat(ptr, "rootfstype=ext4 rw rootwait");
#if defined CONFIG_HB_BOOT_FROM_NAND
			strcat(bootargs_str, " ubi.mtd=2,2048 "__stringify(CONFIG_MTDPARTS_DEFAULT));
#endif
#if defined CONFIG_HB_BOOT_FROM_MMC
			snprintf(tmp, sizeof(tmp), " root=/dev/mmcblk0p%d",
					get_partition_id(system_partition));
			strncat(bootargs_str, tmp,
					sizeof(bootargs_str) - strlen(bootargs_str) - 1);
#endif /*defined CONFIG_HB_BOOT_FROM_MMC*/
		}
		/* Use extra_bootargs to append extra bootargs to bootargs when necessary */
		extra_bootargs = env_get("extra_bootargs");
		if (extra_bootargs != NULL) {
			strncat(bootargs_str, " ", sizeof(bootargs_str) - strlen(bootargs_str) - 1);
			strncat(bootargs_str, extra_bootargs, sizeof(bootargs_str) - strlen(bootargs_str) - 1);
		}
		env_set("bootargs", bootargs_str);
	}

	/* Set Bootcmd */
	memset(tmp, 0, sizeof(tmp));
#if defined CONFIG_HB_BOOT_FROM_MMC
		if (if_secure) {
			snprintf(tmp, sizeof(tmp), "avb_verify; "CONFIG_BOOTCOMMAND,
				 boot_partition, boot_partition);
			env_set("bootcmd", tmp);
		} else {
			snprintf(tmp, sizeof(tmp), CONFIG_BOOTCOMMAND,
				 boot_partition, boot_partition);
#if (CONFIG_BOOTDELAY == 0) && defined(CONFIG_PARALLEL_CPU_CORE_ONE)
		/*The boot image(boot.img) was preinstalled in DDR earlier,
		*so not need reread from eMMC, just run bootm command*/
			if (strcmp(hb_bootreason, REASON_NORMAL) == 0) {
				snprintf(tmp, sizeof(tmp), BOOTCOMMAND_DIRECT_BOOTM);
			}
#endif /*CONFIG_BOOTDELAY=0 && CONFIG_PARALLEL_CPU_CORE_ONE*/
			env_set("bootcmd", tmp);
		}
#elif defined CONFIG_HB_BOOT_FROM_NOR
		/* TODO: Unify bootcmd to get rid of function calls below loading boot */
			boot_mtd = get_mtd_device_nm(boot_mtd_name);
			ret = spi_flash_read(flash, boot_mtd->offset,
						 boot_mtd->size, (void *) BOOTIMG_ADDR);
			if (ret) {
				printf("Error: Read Kernel from SPI Flash failed!\n");
				env_set("bootdelay", "-1");
				return;
			}
#elif defined CONFIG_HB_BOOT_FROM_NAND && !defined(CONFIG_DISTRO_DEFAULTS)
			ubi_part(boot_mtd_name, NULL);
			if (ubi_volume_read("boot", (void *) BOOTIMG_ADDR, 0)) {
				printf("Error: Read Kernel from UBI Volume Boot failed!\n");
				env_set("bootdelay", "-1");
				return;
			}
#endif /*defined CONFIG_HB_BOOT_FROM_NAND && !defined(CONFIG_DISTRO_DEFAULTS)*/

#if (defined(CONFIG_HB_BOOT_FROM_NOR) || defined(CONFIG_HB_BOOT_FROM_NAND)) && !defined(CONFIG_DISTRO_DEFAULTS)
		if (if_secure) {
			env_set("bootcmd", HB_SET_WDT "avb_verify; bootm "__stringify(BOOTIMG_ADDR));
		} else {
			env_set("bootcmd", HB_SET_WDT "bootm "__stringify(BOOTIMG_ADDR));
		}
#endif /*(CONFIG_HB_BOOT_FROM_NOR) || (CONFIG_HB_BOOT_FROM_NAND)*/
}

static void hb_mmc_env_init(void)
{
	char *s;
	char count;
	char cmd[256] = { 0 };
	struct hb_info_hdr *bootinfo = (struct hb_info_hdr*)HB_BOOTINFO_ADDR;
	bool count_pmu_flag = 0;

	if ((strcmp(hb_upmode, UPMODE_AB) == 0) ||
		(strcmp(hb_upmode, UPMODE_GOLDEN) == 0)) {
		if (strcmp(hb_bootreason, REASON_NORMAL) == 0) {
			/* boot_reason is 'normal', normal boot */
			DEBUG_LOG("uboot: normal boot \n");

			veeprom_read(VEEPROM_COUNT_OFFSET, &count,
				VEEPROM_COUNT_SIZE);
			if (count == 'E') {
				/* read count value from pmu register */
				count_pmu_flag = true;
				count = (readl(HB_PMU_SW_REG_23) >> 16) & 0xffff;
			}
			if ((count >= bootinfo->reserved[0]) && (bootinfo->reserved[0] != 0)) {
				if (strcmp(hb_upmode, UPMODE_AB) == 0) {
					/* AB mode, boot system backup partition */
					ota_ab_boot_bak_partition();
				} else {
					/* golden mode, boot recovery system */
					ota_recovery_mode_set(true);
				}
			} else {
				ota_upgrade_flag_check(hb_upmode, hb_bootreason);
			}
		} else if ((strcmp(hb_bootreason, REASON_RECOVERY) == 0) &&
			(strcmp(hb_upmode, UPMODE_GOLDEN) == 0)) {
			/* boot_reason is 'recovery', enter recovery mode */
			env_set("bootdelay", "0");
			ota_recovery_mode_set(false);
		} else {
			ota_upgrade_flag_check(hb_upmode, hb_bootreason);
		}
	}

	//snprintf(logo_addr, sizeof(logo_addr), "0x%x", HB_USABLE_RAM_TOP);
//	env_set("logo_addr", "0x1600000");

	/* init env mem_size */
	s = env_get("mem_size");
	if (s == NULL) {
		// uint32_to_char(sys_sdram_size, cmd);
		snprintf(cmd, sizeof(cmd), "%02x", sys_sdram_size);
		env_set("mem_size", cmd);
	}
}

#ifdef CONFIG_HB_BOOT_FROM_NAND
static void hb_nand_env_init(void)
{
	/* reserve for future use */
	return;
}
#endif /*CONFIG_HB_BOOT_FROM_NAND*/

#ifdef CONFIG_HB_BOOT_FROM_NOR
static void hb_nor_env_init(void)
{
	int ret = 0;
	/* Init SPI NOR if not already probed */
	if (!flash) {
		ret = run_command("sf probe", 0);
		if (ret < 0) {
			printf("Error: flash init failed\n");
			env_set("bootdelay", "-1");
			return;
		}
	}
	return;
}
#endif /*CONFIG_HB_BOOT_FROM_NOR*/
#endif

static int hb_nor_dtb_handle(struct hb_kernel_hdr *config)
{
	char *s = NULL;
	void *dtb_addr, *base_addr = (void *)config;
	uint32_t i = 0;
	uint32_t count = config->dtb_number;
	uint32_t board_type = hb_board_type_get();

	if (count > DTB_MAX_NUM) {
		printf("error: count %02x not support\n", count);
		return -1;
	}

	for (i = 0; i < count; i++) {
		if (board_type == config->dtb[i].board_id) {
			s = (char *)config->dtb[i].dtb_name;
			break;
		}
	}

	if (i == count) {
		printf("error: base_board_id %02x not support\n", board_type);
		return -1;
	}

	printf("fdtimage = %s\n", s);
	env_set("fdtimage", s);

	/* config dtb image */
	dtb_addr = base_addr + sizeof(struct hb_kernel_hdr) + \
		config->dtb[i].dtb_addr;
	memcpy(base_addr, dtb_addr, config->dtb[i].dtb_size);

	return 0;
}

static void hb_usb_dtb_config(void) {
	ulong dtb_addr = env_get_ulong("fdt_addr", 16, FDT_ADDR);
	struct hb_kernel_hdr *head = (struct hb_kernel_hdr *)dtb_addr;

	hb_nor_dtb_handle(head);
}

static void hb_usb_env_init(void)
{
	char *tmp = "send_id;run ddrboot";
	char usb_bootargs[128] = {0};
	snprintf(usb_bootargs, sizeof(usb_bootargs), "%s,%d",
		       CONFIG_BOOTARGS, detect_baud());
	env_set("bootargs", usb_bootargs);
	/* set bootcmd */
	env_set("bootcmd", tmp);
}

#ifndef CONFIG_FPGA_HOBOT
static void hb_env_and_boardid_init(void)
{
	char *s = NULL;
	char upmode[16] = { 0 };
	char boot_reason[64] = { 0 };
	unsigned int boot_mode = hb_boot_mode_get();

	DEBUG_LOG("board_id = %02x\n", hb_board_id);

	/* init env recovery_sys_enable */
	s = env_get("recovery_system");
	if (s && strcmp(s, "disable") == 0) {
		recovery_sys_enable = false;

		/* config resetreason */
		veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
		if (strcmp(boot_reason, REASON_RECOVERY) == 0)
			veeprom_write(VEEPROM_RESET_REASON_OFFSET, REASON_NORMAL,
			VEEPROM_RESET_REASON_SIZE);
	}
	/* init hb_bootreason */
	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
	snprintf(hb_bootreason, sizeof(hb_bootreason), "%s", boot_reason);

	/* init hb_upmode */
	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);
	snprintf(hb_upmode, sizeof(hb_upmode), "%s", upmode);

	/* check if customized bootargs should be used */
	s = env_get("custom_bootargs");
	custom_bootargs = (s == NULL ? false :
						(strcmp(s, "true") == 0) ? true : false);

	/* mmc or nor env init */
#if defined CONFIG_HB_BOOT_FROM_MMC
#ifdef CONFIG_CMD_GPT_RENAME
		/* make sure emmc last partition is properly defined */
		run_command("gpt extend mmc 0", 0);
#endif /*CONFIG_CMD_GPT_RENAME*/
		hb_mmc_env_init();
#elif defined CONFIG_HB_BOOT_FROM_NOR
		/* load nor kernel and dtb */
		hb_nor_env_init();
#elif defined CONFIG_HB_BOOT_FROM_NAND
		hb_nand_env_init();
#endif

	hb_boot_args_cmd_set(boot_mode);
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

static int fdt_get_size(const void *fdt, void *buf, u64 *size)
{
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (size_cells == 2)
		*size = fdt64_to_cpu(*(fdt64_t *)p);
	else
		*size = fdt32_to_cpu(*(fdt32_t *)p);
	p += 4 * size_cells;

	return 0;
}

static int fdt_pack_size(const void *fdt, void *buf, u64 size)
{
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (size_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(size);
	else
		*(fdt32_t *)p = cpu_to_fdt32(size);
	p += 4 * size_cells;

	return p - (char *)buf;
}

static int do_change_model_reserved_size(cmd_tbl_t *cmdtp, int flag,
					 int argc, char * const argv[])
{
	const char *rsv_mem_path;
	const char *ion_path;
	int  model_nodeoffset;
	int  rsv_nodeoffset;
	int  ion_nodeoffset;
	char *prop;
	static char data[1024] __aligned(4);
	void *ion_ptmp;
	int  len;
	void *fdt;
	phys_addr_t fdt_paddr;
	u64 ion_start, old_size, new_size;
	u32 size;
	char *s = NULL;
	int ret;

	DEBUG_LOG("Orign(MAX) bpu model Reserve Mem Size to 64M!!\n");
	if (argc > 1)
		s = argv[1];

	if (s) {
		size = (u32)simple_strtoul(s, NULL, 10);
		if (size == 0 || size > 64)
			return 0;

	} else {
		return 0;
	}

	prop = "reg";
	s = env_get("fdt_addr");
	if (s) {
		fdt_paddr = (phys_addr_t)simple_strtoull(s, NULL, 16);
		fdt = map_sysmem(fdt_paddr, 0);
	} else {
		printf("Can't get fdt_addr !!!");
		return 0;
	}

	rsv_mem_path = "/reserved-memory";
	rsv_nodeoffset = fdt_path_offset(fdt, rsv_mem_path);
	if (rsv_nodeoffset < 0) {
		/*
			* Not found or something else bad happened.
			*/
		printf("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(rsv_nodeoffset));
		return 1;
	}
	ion_path = "/reserved-memory/ion_reserved";
	ion_nodeoffset = fdt_path_offset(fdt, ion_path);
	if (ion_nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(ion_nodeoffset));
		return 1;
	}
	ion_ptmp = (char *)fdt_getprop(fdt, ion_nodeoffset, prop, &len);
	if (len > 1024) {
		printf("prop (%d) doesn't fit in scratchpad!\n",
				len);
		return 1;
	}
	if (!ion_ptmp)
		return 0;

	fdt_get_reg(fdt, ion_ptmp, &ion_start, &old_size);
	new_size = old_size - size * 0x100000;
	len = fdt_pack_reg(fdt, data, ion_start, new_size);

	ret = fdt_setprop(fdt, ion_nodeoffset, prop, data, len);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}
	memset(data, 0, sizeof(data));
	snprintf(data, sizeof(data), "model_rsv@0x%x", rsv_nodeoffset);
	model_nodeoffset = fdt_add_subnode(fdt, rsv_nodeoffset, data);
	if (model_nodeoffset < 0) {
		printf("%s:%d add memory reserved error\n", __func__, __LINE__);
		return 0;
	}

	len = fdt_pack_reg(fdt, data, ion_start + new_size, size * 0x100000);
	ret = fdt_setprop(fdt, model_nodeoffset, prop, data, len);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return 1;
	}
	return 0;
}

U_BOOT_CMD(
	model_reserved_modify,	2,	0,	do_change_model_reserved_size,
	"Change BPU Reserved Mem Size(Mbyte)",
	"-model_modify 100"
);

static void change_ion_cma_status(void *fdt, uint16_t status)
{
	const char *cma_path = "/reserved-memory/ion_cma";
	const char *reserved_path = "/reserved-memory/ion_reserved";
	const char *prop = "status";
	char *cma_status;
	char *reserved_status;
	int cma_nodeoffset, reserved_nodeoffset;
	int ret;

	if (status == ion_cma_status) {
		return;
	}
	if (status > 0) {
		cma_status = "okay";
		reserved_status = "disabled";
	} else {
		cma_status = "disabled";
		reserved_status = "okay";
	}

	cma_nodeoffset = fdt_path_offset(fdt, cma_path);
	reserved_nodeoffset = fdt_path_offset(fdt, reserved_path);
	if ((cma_nodeoffset < 0) || (reserved_nodeoffset < 0)) {
		return;
	}

	ret = fdt_setprop(fdt, cma_nodeoffset, prop,
			cma_status, strlen(cma_status));
	if (ret < 0) {
		DEBUG_LOG("ion cma set status fdt_setprop(): %s\n",
				fdt_strerror(ret));
		return;
	}

	ret = fdt_setprop(fdt, reserved_nodeoffset, prop,
			reserved_status, strlen(reserved_status));
	if (ret < 0) {
		DEBUG_LOG("ion reserved set status fdt_setprop(): %s\n",
				fdt_strerror(ret));
		return;
	}

	ion_cma_status = status;
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
	u32 tmp_val, size = 0;
	u16 ion_type = 1;
	char *s = NULL;
	int ret;

	if (argc > 1) {
		s = argv[1];
		DEBUG_LOG("MAX Ion Reserve/CMA Mem Size to 1024M\n");
		if (s) {
			tmp_val = (u32)simple_strtoul(s, NULL, 10);
			if ((tmp_val == 0) || (tmp_val == 1)) {
				ion_type = tmp_val;
			} else {
				size = tmp_val;
			}
		} else {
			return 0;
		}
	}

	if (argc > 2) {
		s = argv[2];
		if (s) {
			ion_type = (u16)simple_strtoul(s, NULL, 10);
		}
	}

	s = env_get("fdt_addr");
	if (s) {
		fdt_paddr = (phys_addr_t)simple_strtoull(s, NULL, 16);
		fdt = map_sysmem(fdt_paddr, 0);
	} else {
		printf("Can't get fdt_addr !!!");
		return 0;
	}

	change_ion_cma_status(fdt, ion_type);
	DEBUG_LOG("Ion Heap type %s Mem\n",
			ion_cma_status ? "CMA" : "Reserved");

	if (size == 0 || size > 1024) {
		return 0;
	}

	if (size < 64)
		size = 64;

	if (ion_type == 0) {
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
		DEBUG_LOG("Orign Ion Reserve Mem Size is %dM\n",
				(int)(old_size / 0x100000));

		len = fdt_pack_reg(fdt, data, ion_start, size * 0x100000);

		ret = fdt_setprop(fdt, nodeoffset, prop, data, len);
		if (ret < 0) {
			printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
			return 1;
		}

		DEBUG_LOG("Change Ion Reserve Mem Size to %dM\n", size);
	} else {
		path = "/reserved-memory/ion_cma";

		nodeoffset = fdt_path_offset (fdt, path);
		if (nodeoffset < 0) {
			/*
			 * Not found or something else bad happened.
			 */
			printf ("libfdt fdt_path_offset() returned %s\n",
					fdt_strerror(nodeoffset));
			return 1;
		}
		ptmp = (char *)fdt_getprop(fdt, nodeoffset, "alloc-ranges", &len);
		if (len > 1024) {
			printf("prop (%d) doesn't fit in scratchpad!\n",
					len);
			return 1;
		}
		if (!ptmp)
			return 0;

		fdt_get_reg(fdt, ptmp, &ion_start, &old_size);

		len = fdt_pack_reg(fdt, data, ion_start, size * 0x100000);

		ret = fdt_setprop(fdt, nodeoffset, "alloc-ranges", data, len);
		if (ret < 0) {
			printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
			return 1;
		}

		ptmp = (char *)fdt_getprop(fdt, nodeoffset, "size", &len);
		if (len > 1024) {
			printf("prop (%d) doesn't fit in scratchpad!\n",
					len);
			return 1;
		}
		if (!ptmp)
			return 0;

		fdt_get_size(fdt, ptmp, &old_size);
		DEBUG_LOG("Orign Ion CMA Mem Size is %dM\n",
				(int)(old_size / 0x100000));

		len = fdt_pack_size(fdt, data, size * 0x100000);

		ret = fdt_setprop(fdt, nodeoffset, "size", data, len);
		if (ret < 0) {
			printf ("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
			return 1;
		}

		DEBUG_LOG("Change Ion CMA Reserve Mem Size to %dM\n", size);
	}

	return 0;
}

U_BOOT_CMD(
	ion_modify,	3,	0,	do_change_ion_size,
	"Change ION Reserved Mem Size(Mbyte)",
	"-ion_modify 100"
);

static int do_fdt_enable(cmd_tbl_t *cmdtp, int flag, int argc,
						 char * const argv[])
{
	const char *path;
	int  ret;
	char *status_val = NULL;
	char cmd[128];

	if (argc == 1)
		return 0;

	if (argc < 3 || argc % 2 == 0) {
		printf("invalid param\n");
		return CMD_RET_USAGE;
	}

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("fdt addr ${fdt_addr} failed:%d\n", ret);
		return CMD_RET_FAILURE;
	}

	for (int i = 1; i < argc; i += 2) {
		if (!strncmp(argv[i], "enable", strlen("enable"))) {
			status_val = "okay";
		} else if (!strncmp(argv[i], "disable", strlen("disable"))) {
			status_val = "disabled";
		} else {
			printf("Please use \"enable\" or \"disable\"\n");
			return CMD_RET_USAGE;
		}

		path = argv[i + 1];
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "fdt set %s status %s", path, status_val);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Set property failed : %d!, path: %s\n", ret, path);
			return CMD_RET_FAILURE;
		}
	}

	DEBUG_LOG("Changed status Done\n");

	return 0;
}

U_BOOT_CMD(
	fdt_enable,	1 + 8 * 2,	0,	do_fdt_enable,
	"Enable/disable the node specified at <path>",
	"-fdt_enable enable/disable <path>"
);

#ifndef CONFIG_HB_QUICK_BOOT
static int do_fix_mmc_buswidth(cmd_tbl_t *cmdtp, int flag, int argc,
						 char * const argv[])
{
	int  ret;
	char *mmc_buswidth = "4";
	char cmd[128];

	if (argc == 2)
		mmc_buswidth = argv[1];
	/*
	 * efuse bank28 bit[0:3] and bit[20:23] is 1
	 * to set emmc width is 4
	 */
	else if (((api_efuse_read_data(HB_BANK) >> 20) & 0xF) != EMMC_BW_4 ||
		 (api_efuse_read_data(HB_BANK) & 0xF) != HORIZON_BOARD)
		return 0;

	snprintf(cmd, sizeof(cmd), "fdt addr ${fdt_addr}");
	ret = run_command(cmd, 0);
	if (ret)
		return CMD_RET_FAILURE;

	snprintf(cmd, sizeof(cmd), "fdt set /soc/dwmmc@A5010000 bus-width <%s>",
			 (!strcmp(mmc_buswidth, "1") ? "1" :
			 (!strcmp(mmc_buswidth, "8") ? "8" : "4")));
	ret = run_command(cmd, 0);

	if (ret != 0) {
		printf("Set property failed : %d!\n", ret);
		return CMD_RET_FAILURE;
	} else {
		printf("Set mmc buswidth to %s\n", mmc_buswidth);
	}

	return 0;
}

U_BOOT_CMD(
	fix_mmc_buswidth,	2,	0,	do_fix_mmc_buswidth,
	"fix_mmc_buswidth <mmc_buswidth>",
	"fix mmc buswidth for older SOMs"
);
#endif /*CONFIG_HB_QUICK_BOOT*/

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
#if defined(CONFIG_NR_DRAM_BANKS)
	printf("gd mem size is %02llx\n", gd->bd->bi_dram[0].size);

#if defined(PHYS_SDRAM_2) && defined(CONFIG_MAX_MEM_MAPPED)
	/* set mem size */
	gd->bd->bi_dram[0].size =
		size > CONFIG_MAX_MEM_MAPPED ? CONFIG_MAX_MEM_MAPPED : size;
	gd->bd->bi_dram[1].start =
		size > CONFIG_MAX_MEM_MAPPED ? PHYS_SDRAM_2 : 0;
	gd->bd->bi_dram[1].size =
		size > CONFIG_MAX_MEM_MAPPED ? PHYS_SDRAM_2_SIZE : 0;
#else
	/* set mem size */
	gd->bd->bi_dram[0].size = size;
#endif
#endif

	return 0;
}

U_BOOT_CMD(
	mem_modify,	2,	0,	do_set_tag_memsize,
	"Change DDR Mem Size(Mbyte)",
	"-mem_modify 0x40000000"
);
#endif
#ifdef HB_AUTORESET
static void prepare_autoreset(void)
{
	printf("prepare for auto reset test ...\n");
	mdelay(50);

	env_set("bootcmd_bak", env_get("bootcmd"));
	env_set("bootcmd", "reset");
	return;
}
#endif

#if 0
//START4[prj_j2quad]
//#if defined(CONFIG_X2_QUAD_BOARD)

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
#endif

#ifdef CONFIG_CMD_OTA_WRITE
static int flash_write_partition(char *partition, int partition_offset,
				int partition_size)
{
	int ret = CMD_RET_SUCCESS;
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "mtd erase %s %s",
			 partition, "0x0");
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		printf("mtd erase partition %s failed\n", partition);
	snprintf(cmd, sizeof(cmd), "mtd write %s %x 0x0 %x", partition,
				partition_offset, partition_size);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		printf("mtd write partition %s failed\n", partition);
	return ret;
}

static int do_burn_flash(cmd_tbl_t *cmdtp, int flag,
	int argc, char * const argv[])
{
#define MAX_MTD_PART_NUM 16
#define MAX_MTD_PART_NAME 128
	struct mtd_info *mtd, *part;
	__maybe_unused char cmd[512];
	int dev_nb = 0, ret = CMD_RET_SUCCESS;
	u32 img_addr, img_size;
	int img_remain;
	/*[0] - bl_size; [1] - boot_size; [2] - system_size; [3] - userdata_size*/
	char *s1 = NULL, *s2 = NULL, *target_part = "", *flash_type = NULL;
	if (argc < 4) {
		printf("flash_type image_addr and img_size must be given, abort\n");
		return CMD_RET_USAGE;
	} else if (argc == 5) {
		target_part = argv[2];
		s1 = argv[3];
		s2 = argv[4];
	} else {
		s1 = argv[2];
		s2 = argv[3];
	}
	flash_type = argv[1];
	/* TODO: Implement CS */
	img_addr = (u32)simple_strtoul(s1, NULL, 16);
	img_size = (u32)simple_strtoul(s2, NULL, 16);
	if (img_addr == 0) {
		printf("0 address not allowed!\n");
		return CMD_RET_FAILURE;
	}
	img_remain = img_size;
	mtd = __mtd_next_device(0);
	/* Ensure all devices (and their partitions) are probed */
	if (!mtd || list_empty(&(mtd->partitions))) {
		mtd_probe_devices();
		mtd_for_each_device(mtd) {
			dev_nb++;
		}
		if (!dev_nb && !strcmp(flash_type, "nor")) {
			run_command("sf probe", 0);
			mtd_probe_devices();
			mtd_for_each_device(mtd) {
				dev_nb++;
			}
			if (!dev_nb) {
				printf("No MTD device found, abort\n");
				return CMD_RET_FAILURE;
			}
		}
		mtd = __mtd_next_device(0);
	}

	if (mtd == NULL) {
		printf("No MTD device found!\n");
		return -ENODEV;
	}

	if (!strcmp(flash_type, "nand")) {
		if (mtd->type != MTD_NANDFLASH) {
			printf("NAND flash not found, abort!\n");
			return CMD_RET_FAILURE;
		}
	} else if (!strcmp(flash_type, "nor")) {
		if (mtd->type != MTD_NORFLASH) {
			printf("NOR flash not found, abort!\n");
			return CMD_RET_FAILURE;
		}
	} else {
		printf("Flash type %s not supported, abort!\n", flash_type);
		return CMD_RET_FAILURE;
	}

	if ((argc == 5) && list_empty(&mtd->partitions)) {
		printf("No MTD Partition found, abort!\n");
		return CMD_RET_FAILURE;
	}

	ret = 0;
	if ((argc == 5) && strcmp(target_part, "all")) {
		part = get_mtd_device_nm(target_part);
		if (!IS_ERR_OR_NULL(part)) {
			if (img_size > part->size) {
				printf("Image size larger than partition size, abort!\n");
				return CMD_RET_FAILURE;
			} else {
				printf(
					"Burning image of size 0x%llx from address: 0x%x to %s\n",
					part->size, img_addr, flash_type);
				ret = flash_write_partition(part->name,
								img_addr,
								part->size);
			}
		} else {
			printf("Partition %s not found, abort!\n", target_part);
			ret = CMD_RET_FAILURE;
		}
	} else {
		if (mtd->size < img_size) {
			printf("Image size is larger than Flash size, abort!\n");
			return CMD_RET_FAILURE;
		}
		if (!strcmp(target_part, "all")) {
			list_for_each_entry(part, &mtd->partitions, node) {
				printf("Burning image of size 0x%llx from address: 0x%llx to %s\n",
						(part->size < img_remain) ? part->size : img_remain,
						img_addr + part->offset, flash_type);
				ret = flash_write_partition(part->name, img_addr + part->offset,
							(part->size < img_remain) ? part->size : img_remain);
				img_remain -= part->size;
				if (img_remain <= 0 || !strcmp("system", part->name)) {
					break;
				}
			}
		} else {
			ret = flash_write_partition(mtd->name, img_addr, img_size);
		}
	}

	printf("Burn Flash Done!\n");
	return ret;
}

U_BOOT_CMD(
	burn_flash,	5,	0,	do_burn_flash,
	"Burn Image at [addr] in DDR with [size] in bytes(hex) to nand/nor flash",
	"<flash_type> [partition - optional] <addr_in_mem> <img_size>\n"
	"          flash_type : \"nor\" and \"nand\" are supported\n"
	"          partition  : \"all\": until \"system\" will be updated\n"
	"                       \"not specified\": whole flash will be erased"
);
#endif /*CONFIG_CMD_OTA_WRITE*/

#ifndef	CONFIG_FPGA_HOBOT
#if defined(HB_SWINFO_BOOT_OFFSET)
static int hb_swinfo_boot_uboot_check(void)
{
	uint32_t s_magic, s_boot, s_f = 0;
	void *s_addr;

	s_magic = readl((void*)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addr = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_BOOT_OFFSET);
		s_f = 1;
	} else {
		s_addr = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_BOOT_OFFSET);
	}
	s_boot = readl(s_addr) & 0xF;
	if (s_boot == HB_SWINFO_BOOT_UBOOTONCE ||
		s_boot == HB_SWINFO_BOOT_UBOOTWAIT) {
		puts("wait for swinfo boot: ");
		if (s_boot == HB_SWINFO_BOOT_UBOOTONCE) {
			puts("ubootonce\n");
			writel(s_boot & (~0xF), s_addr);
			if (s_f)
				flush_dcache_all();
		} else {
			puts("ubootwait\n");
		}
		return 1;
	}

	return 0;
}
#endif

#if defined(HB_SWINFO_DUMP_OFFSET)
static int hb_swinfo_dump_check(void)
{
	uint32_t s_magic, s_boot, s_dump;
	void *s_addrb, *s_addrd;
	uint8_t d_ip[5];
	char dump[128];
	char *s = dump;
	const char *dcmd, *ddev;
	uint32_t dmmc, dpart, dusb, dusbpart;
	unsigned int dump_sdram_size = sys_sdram_size - CONFIG_SYS_SDRAM_BASE;
	char *dir = "";

	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addrb = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_DUMP_OFFSET);
	} else {
		s_addrb = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_DUMP_OFFSET);
	}
	s_boot = readl(s_addrb);
	s_dump = readl(s_addrd);
	if (s_boot == HB_SWINFO_BOOT_UDUMPTF ||
		s_boot == HB_SWINFO_BOOT_UDUMPEMMC) {
		stored_dumptype = 1;
		if (s_boot == HB_SWINFO_BOOT_UDUMPTF) {
			ddev = "tfcard";
			dcmd = "part list mmc 1; fatwrite";
			dmmc = 1;
			dpart = 1;
		} else {
			ddev = "emmc";
			dcmd = "ext4write";
			dir  = "/log";
			dmmc = 0;
			dpart = get_partition_id("userdata");
		}
		printf("swinfo dump ddr 0x%x -> %s:p%d\n", dump_sdram_size, ddev, dpart);
		s += sprintf(s, "mmc rescan; ");
		s += sprintf(s, "%s mmc %x:%x 0x%x %s/dump_ddr_%x.img 0x%x",
				dcmd, dmmc, dpart, CONFIG_SYS_SDRAM_BASE, dir,
				dump_sdram_size, dump_sdram_size);

		env_set("dumpcmd", dump);
	} else if (s_boot == HB_SWINFO_BOOT_UDUMPUSB) {
		stored_dumptype = 1;
		ddev = "usb";
		dcmd = "fatwrite";
		dusb = 0;

		printf("swinfo dump ddr 0x%x -> %s\n", dump_sdram_size, ddev);
		s += sprintf(s, "usb start; ");
		s += sprintf(s, "usb part %d; ", dusb);
		run_command(dump, 0);
		dusbpart = get_dos_firstpartition_id();
		memset(dump, 0, 128);
		s = dump;
		s += sprintf(s, "%s usb %d:%d 0x%x dump_ddr_%x.img 0x%x;fatls usb %d:%d /",
				dcmd, dusb, dusbpart, CONFIG_SYS_SDRAM_BASE,
				dump_sdram_size, dump_sdram_size, dusb, dusbpart);

		env_set("dumpcmd", dump);
	} else if (s_boot == HB_SWINFO_BOOT_UDUMPFASTBOOT) {
		stored_dumptype = 1;
		s += sprintf(s, "fastboot 0");

		env_set("dumpcmd", dump);
		printf("enter fastboot ramdump mode\n");
		printf("please use \"fastboot oem ramdump\" command in pc\n");
	} else if (s_dump) {
		stored_dumptype = 2;
		d_ip[0] = (s_dump >> 24) & 0xff;
		d_ip[1] = (s_dump >> 16) & 0xff;
		d_ip[2] = (s_dump >> 8) & 0xff;
		d_ip[3] = s_dump & 0xff;
		d_ip[4] = (d_ip[3] == 1) ? 10 : 1;
		printf("swinfo dump ddr 0x%x %d.%d.%d.%d -> %d\n",
			   dump_sdram_size, d_ip[0], d_ip[1], d_ip[2],
			   d_ip[4], d_ip[3]);
		s += sprintf(s, "setenv ipaddr %d.%d.%d.%d;",
					 d_ip[0], d_ip[1], d_ip[2], d_ip[4]);
		s += sprintf(s, "setenv serverip %d.%d.%d.%d;",
					 d_ip[0], d_ip[1], d_ip[2], d_ip[3]);
		s += sprintf(s, "tput 0x%x 0x%x dump_ddr_%x.img",
					 CONFIG_SYS_SDRAM_BASE,
					 dump_sdram_size, dump_sdram_size);
		env_set("dumpcmd", dump);
	} else {
		stored_dumptype = 0;
	}

	return stored_dumptype;
}

static int hb_swinfo_dump_donecheck(int retc)
{
	uint32_t s_magic, s_boot, s_f = 0;
	void *s_addrb, *s_addrd;

	if (retc) {
		printf("swinfo dump ddr error %d.\n", retc);
		return retc;
	}

	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addrb = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_DUMP_OFFSET);
		s_f = 1;
	} else {
		s_addrb = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_DUMP_OFFSET);
	}
	if (stored_dumptype == 1) {
		s_boot = readl(s_addrb);
		writel(s_boot & (~0xF), s_addrb);
	} else if (stored_dumptype == 2) {
		writel(0x00, s_addrd);
	} else {
		s_f = 0;
	}
	if (s_f)
		flush_dcache_all();

	printf("swinfo dump ddr done.\n");
	return 0;
}
#endif
#endif

#if 0
static void misc()
{
	//START4[prj_j2quad]
//#if defined(CONFIG_X2_QUAD_BOARD)
	if (hb_get_boardid()==QUAD_BOARD_ID){
#if defined(CONFIG_J2_LED)
		j2_led_init();
#endif
		reboot_notify_to_mcu();
		printf("=>j2quad<=");
	}
	if ((hb_get_boardid() == J2_MM_BOARD_ID) ||
			(hb_get_boardid() == J2_MM_S202_BOARD_ID)) {
		env_set("kernel_addr", "0x400000");
	}
//#endif
//END4[prj_j2quad]
}
#endif
#if !defined(CONFIG_FPGA_HOBOT) && defined(CONFIG_CMD_SWINFO)
static void hb_swinfo_boot(void)
{
	int retc;
	const char *s = NULL;
	int stored_bootdelay = -1;
#if defined(HB_SWINFO_DUMP_OFFSET)
	if(hb_swinfo_dump_check()) {
		s = env_get("dumpcmd");
		stored_bootdelay = 0;
		if (cli_process_fdt(&s))
			cli_secure_boot_cmd(s);
	}
#endif

#if defined(HB_SWINFO_BOOT_OFFSET)
	if(hb_swinfo_boot_uboot_check()) {
		env_set("ubootwait", "wait");
		return;
	} else {
		env_set("ubootwait", NULL);
	}
#endif
	if (stored_bootdelay == 0 && s) {
#if defined(CONFIG_HB_WATCHDOG) && \
		!(defined(CONFIG_HB_AP_BOOT) || defined(CONFIG_HB_YMODEM_BOOT))
		hb_wdt_stop();
#endif
		retc = run_command_list(s, -1, 0);
#if defined(HB_SWINFO_DUMP_OFFSET)
		if(stored_dumptype)
			hb_swinfo_dump_donecheck(retc);
#endif
		return;
	}
}

#endif

#if defined(CONFIG_FASTBOOT) || defined(CONFIG_USB_FUNCTION_MASS_STORAGE) \
	|| defined(CONFIG_DFU_OVER_USB)
int setup_boot_action(int boot_mode)
{
	void *reg = (void *)HB_PMU_SW_REG_04;
	int boot_action = readl(reg);

	if (boot_mode != PIN_2ND_USB && hb_fastboot_key_pressed()) {
		printf("enter fastboot mode(reuse bootsel[15]: 1)\n");
		boot_action = BOOT_FASTBOOT;
	}

	debug("%s: boot action 0x%08x\n", __func__, boot_action);

	/* Clear boot mode */
	writel(BOOT_NORMAL, reg);
	if (boot_action == BOOT_FASTBOOT ||
	    boot_action == BOOT_UMS ||
	    boot_action == BOOT_DFU ||
	    boot_action == BOOT_UFU) {
#if defined(CONFIG_HB_WATCHDOG) && \
		!(defined(CONFIG_HB_AP_BOOT) || defined(CONFIG_HB_YMODEM_BOOT))
		hb_wdt_stop();
#endif
	}
	switch (boot_action) {
	case BOOT_FASTBOOT:
		/* currently only nand boot to flash nand, others all flash emmc */
		printf("%s: enter fastboot!\n", __func__);
		env_set("preboot", "fastboot usb 0");
		break;
	case BOOT_UMS:
		/* ums currently only support emmc */
		printf("%s: enter emmc UMS!\n", __func__);
		env_set("preboot", "setenv preboot; ums 0 mmc 0");
		break;
	case BOOT_DFU:
		/* currently only nand boot to flash nand, others all flash emmc */
		if (boot_mode == PIN_2ND_NAND) {
			printf("%s: enter spi-nand0 DFU!\n", __func__);
			env_set("preboot", "setenv preboot; dfu 0 mtd spi-nand0");
		} else {
			printf("%s: enter emmc DFU!\n", __func__);
			env_set("preboot", "setenv preboot; dfu 0 mmc 0");
		}
		break;
	case BOOT_UFU:
		/* ufu currently use emmc 0:1 as storage, later will use ddr */
		printf("%s: enter emmc UFU!\n", __func__);
		env_set("preboot", "setenv preboot; ufu 0 mmc 0:1");
		break;
	}

	return 0;
}
#endif

int board_early_init_r(void)
{
#ifdef HB_AUTOBOOT
        boot_stage_mark(1);
#endif
#ifdef CONFIG_HB_BIFSD
	initr_bifsd();
#endif
	return 0;
}

int board_early_init_f(void)
{
#ifdef CONFIG_TARGET_XJ3
#ifdef SET_QOS_IN_UBOOT
	update_qos();
#endif // SET_QOS_IN_UBOOT
	init_io_vol();
	vio_pll_init();
#endif
	bif_change_reset2gpio();
#ifdef ENABLE_BIFSPI
	writel(0xFED10000, BIF_SHARE_REG_BASE);
#endif
#ifdef HB_AUTOBOOT
        boot_stage_mark(0);
#endif
#if defined(CONFIG_SYSRESET)
	print_resetinfo();
#endif
	return 0;
}

static void base_board_gpio_test(void)
{
	uint32_t  base_board_id = hb_base_board_type_get();

	switch (base_board_id) {
	case BASE_BOARD_X3_DVB:
		DEBUG_LOG("base board type: X3 DVB\n");
		break;
	case BASE_BOARD_J3_DVB:
		DEBUG_LOG("base board type: J3 DVB\n");
		break;
	case BASE_BOARD_CVB:
		DEBUG_LOG("base board type: CVB\n");
		break;
	case BASE_BOARD_X3_SDB:
		DEBUG_LOG("base board type: X3 SDB\n");
		break;
	default:
		DEBUG_LOG("base board type not support\n");
		break;
	}

}

#ifdef CONFIG_DISTRO_DEFAULTS
static char * get_dtb_name(void)
{
	uint32_t som_type = hb_som_type_get();
	char *dtb_name = NULL;
	switch (som_type) {
	case SOM_TYPE_X3:
		dtb_name = "hobot-x3-dvb.dtb";
		break;
	case SOM_TYPE_J3:
		dtb_name = "hobot-j3-dvb.dtb";
		break;
	case SOM_TYPE_X3SDB:
		dtb_name = "hobot-x3-sdb.dtb";
		break;
	case SOM_TYPE_X3SDBV4:
		dtb_name = "hobot-x3-sdb_v4.dtb";
		break;
	case SOM_TYPE_X3PI:
		dtb_name = "hobot-x3-pi.dtb";
		break;
	case SOM_TYPE_X3PIV2:
		dtb_name = "hobot-x3-pi.dtb";
		break;
	case SOM_TYPE_X3E:
		dtb_name = "hobot-x3-sdb.dtb";
		break;
	default:
		printf("Unsupported board ID!!\n");
		dtb_name = "none";
		break;
	}
	env_set("fdtfile", dtb_name);
	printf("dtb_name:%s\n", dtb_name);
	return dtb_name;
}
#endif

static void boot_src_test(void)
{
	uint32_t boot_src = hb_boot_mode_get();

	switch (boot_src) {
	case PIN_2ND_EMMC:
		DEBUG_LOG("bootmode: EMMC\n");
		break;
	case PIN_2ND_NAND:
		DEBUG_LOG("bootmode: NAND\n");
		break;
	case PIN_2ND_AP:
		DEBUG_LOG("bootmode: AP\n");
		break;
	case PIN_2ND_USB:
	case PIN_2ND_USB_BURN:
		DEBUG_LOG("bootmode: USB\n");
		break;
	case PIN_2ND_NOR:
		DEBUG_LOG("bootmode: NOR\n");
		break;
	case PIN_2ND_UART:
		DEBUG_LOG("bootmode: UART\n");
		break;
	default:
		DEBUG_LOG("bootmode not support\n");
		break;
}
}

int last_stage_init(void)
{
	int boot_mode = hb_boot_mode_get();

#ifndef CONFIG_FPGA_HOBOT
	disable_cnn();
#endif
#ifdef CONFIG_DISTRO_DEFAULTS
	get_dtb_name();
#endif

#if defined(CONFIG_MMC) && !defined(CONFIG_HB_QUICK_BOOT)
	/* for determining mmc bus-width from environment */
	run_command("mmc rescan", 0);
#endif
	if (readl(HB_PMU_SW_REG_23) != 0x74726175) {
		if (veeprom_init() && boot_mode == PIN_2ND_NAND) {
			return 0;
		}
	}
#ifdef ENABLE_BIFSPI
	bif_recover_reset_func();
	apbooting();
#endif
#ifdef	CONFIG_AP_CP_COMN_MODE
	hb_ap_communication();
#endif

	sw_efuse_set_register();
	base_board_gpio_test();

	if (readl(HB_PMU_SW_REG_23) != 0x74726175)
		boot_src_test();
	else
		DEBUG_LOG("bootmode: UART\n");

#ifdef CONFIG_HBOT_SECURE_ENGINE
	/* spacc and pka init */
	spacc_init();
	pka_init();
#endif

#ifdef HB_AUTOBOOT
	boot_stage_mark(2);
	wait_start();
#endif

#ifndef	CONFIG_FPGA_HOBOT
	if (readl(HB_PMU_SW_REG_23) != 0x74726175) {
#ifndef CONFIG_DOWNLOAD_MODE
		if ((boot_mode == PIN_2ND_NOR) || (boot_mode == PIN_2ND_NAND) \
			|| (boot_mode == PIN_2ND_EMMC))
			hb_env_and_boardid_init();

		if (boot_mode == PIN_2ND_USB) {
			hb_usb_dtb_config();

			hb_usb_env_init();
		}
#endif
	}
#endif

#ifdef HB_AUTORESET
	prepare_autoreset();
#endif

#if !defined(CONFIG_FPGA_HOBOT) && defined(CONFIG_CMD_SWINFO)
	hb_swinfo_boot();
#endif

#if defined(CONFIG_FASTBOOT) || defined(CONFIG_USB_FUNCTION_MASS_STORAGE)
	setup_boot_action(boot_mode);
#endif

//	misc();
	if (readl(HB_PMU_SW_REG_23) != 0x74726175) {
#ifdef CONFIG_TARGET_XJ3
		hb_mipi_panel_reset();
		/*both pvt and gptp use sys_pclk,
		 * pvt can work properly when sys_pclk is 250M*/
		change_sys_pclk_250M();
#endif
	}
	return 0;
}
