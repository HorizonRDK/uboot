/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <sata.h>
#include <stdarg.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/string.h>
#include <malloc.h>
#include <mapmem.h>
#include <asm/io.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_sysctrl.h>
#include <hb_info.h>
#include <asm/arch-x2/ddr.h>
#include "xj3_set_pin.h"
#include <hb_utils.h>

extern struct pin_info pin_info_array[];
extern uint32_t pin_info_len;
static uint8_t xj3_all_pin_type[HB_PIN_MAX_NUMS] = {
	/* GPIO0[0-7] */
	PIN_INVALID, PULL_TYPE1, PULL_TYPE1, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO0[8-15] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE1, PULL_TYPE1, PIN_INVALID, PULL_TYPE2, PIN_INVALID, PULL_TYPE2,
	/* GPIO1[0-7] */
	PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO1[8-15] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO2[0-7] */
	PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1,
	/* GPIO2[8-15] */
	PULL_TYPE1, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO3[0-7] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO3[8-15] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO4[0-7] */
	PULL_TYPE1, PULL_TYPE1, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO4[8-15] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO5[0-7] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO5[8-15] */
	PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE1,
	/* GPIO6[0-7] */
	PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1,
	/* GPIO6[8-15] */
	PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE1, PULL_TYPE2,
	/* GPIO7[0-7] */
	PULL_TYPE2, PIN_INVALID, PULL_TYPE1, PULL_TYPE1, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2, PULL_TYPE2,
	/* GPIO7[8] */
	PULL_TYPE1,
};

static uint8_t xj3_pin_type(uint32_t pin)
{
	if (pin >= HB_PIN_MAX_NUMS)
		return PIN_INVALID;
	return xj3_all_pin_type[pin];
}

static void xj3_set_pin_func(char pin, unsigned int val)
{
	uint32_t reg = 0;
	uint32_t offset = 0;
	uint32_t mask = 3;

	offset = pin * 4;
	reg = reg32_read(X2A_PIN_SW_BASE + offset);
	reg = (reg & ~mask) | (val & mask);
	reg32_write(X2A_PIN_SW_BASE + offset, reg);
}

static void xj3_set_pin_dir(char pin, int dir)
{
	uint32_t reg = 0;
	uint32_t offset = 0;

	if (dir) {
		/* set pin to ouput */
		offset = (pin / 16) * 0x10 + 0x08;
		reg = reg32_read(X2_GPIO_BASE + offset);
		reg |= (1 << ((pin % 16) + 16));
		reg32_write(X2_GPIO_BASE + offset, reg);
	} else {
		/* set pin to input */
		offset = (pin / 16) * 0x10 + 0x08;
		reg = reg32_read(X2_GPIO_BASE + offset);
		reg &= (~(1 << ((pin % 16) + 16)));
		reg32_write(X2_GPIO_BASE + offset, reg);
	}
}

static void xj3_set_pin_value(char pin, unsigned int val)
{
	uint32_t reg = 0;
	uint32_t offset = 0;

	/* set pin output value*/
	offset = (pin / 16) * 0x10 + 0x08;
	reg = reg32_read(X2_GPIO_BASE + offset);
	if (val == 1) {
		reg |= (1 << (pin % 16));
	} else {
		reg &= (~(1 << (pin % 16)));
	}

	reg32_write(X2_GPIO_BASE + offset, reg);
}

static void xj3_set_gpio_pull(char pin, enum pull_state state, uint32_t pin_type)
{
	uint32_t reg = 0;
	uint32_t offset = 0;

	offset = pin * 4;
	reg = reg32_read(X2A_PIN_SW_BASE + offset);
	reg |= 3;
	if (pin_type == PULL_TYPE1) {
		switch (state) {
		case NO_PULL:
			reg &= PIN_TYPE1_PULL_DISABLE;
			break;
		case PULL_UP:
			reg |= PIN_TYPE1_PULL_ENABLE;
			reg |= PIN_TYPE1_PULL_UP;
			break;
		case PULL_DOWN:
			reg |= PIN_TYPE1_PULL_ENABLE;
			reg &= PIN_TYPE1_PULL_DOWN;
			break;
		default:
			break;
		}
		reg32_write(X2A_PIN_SW_BASE + offset, reg);
	} else if (pin_type == PULL_TYPE2) {
		switch (state) {
		case NO_PULL:
			reg &= PIN_TYPE2_PULL_UP_DISABLE;
			reg &= PIN_TYPE2_PULL_DOWN_DISABLE;
			break;
		case PULL_UP:
			reg |= PIN_TYPE2_PULL_UP_ENABLE;
			reg &= PIN_TYPE2_PULL_DOWN_DISABLE;
			break;
		case PULL_DOWN:
			reg |= PIN_TYPE2_PULL_DOWN_ENABLE;
			reg &= PIN_TYPE2_PULL_UP_DISABLE;
			break;
		default:
			break;
		}
		reg32_write(X2A_PIN_SW_BASE + offset, reg);
	}
}

void dump_pin_info(void)
{
	uint32_t i = 0;
	uint32_t reg = 0;
	for (i = 0; i < 8; i++) {
		reg = X2_GPIO_BASE + 0x10 * i + 0x8;
		printf("reg[0x%x] = 0x%x\n", reg, reg32_read(reg));
		reg = X2_GPIO_BASE + 0x10 * i + 0xc;
		printf("reg[0x%x] = 0x%x\n", reg, reg32_read(reg));
	}
	for (i = 0; i < HB_PIN_MAX_NUMS; i++) {
		reg = X2A_PIN_SW_BASE + i * 4;
		printf("pin[%d] status reg[0x%x] = 0x%x\n", i, reg, reg32_read(reg));
	}
}


#ifdef CONFIG_VIDEO_HOBOT_XJ3
uint8_t get_lt8618_rst(void)
{
	uint32_t som_type = hb_som_type_get();
	uint8_t reset_pin = -1;
	switch (som_type)
	{
	case SOM_TYPE_X3PI:
		reset_pin = 117;
		break;
	case SOM_TYPE_X3PIV2_1:
		reset_pin = 60;
		break;
	case SOM_TYPE_X3CM:
		reset_pin = 115;
		break;
	default:
		printf("%s :There is nothing to do,return!", __func__);
		break;
	}
	return reset_pin;
}
#endif

void xj3_set_pin_info(void)
{
	int32_t i = 0;
	uint32_t pin_type = 0;
#ifdef CONFIG_VIDEO_HOBOT_XJ3
	uint8_t hdmi_rst = get_lt8618_rst();
	if(hdmi_rst == -1)
		printf("get lt8618 rst pin failed!\n");
#endif
	for (i = 0; i < pin_info_len; i++) {
		pin_type = xj3_pin_type(pin_info_array[i].pin_index);
#ifdef CONFIG_VIDEO_HOBOT_XJ3
		if(pin_info_array[i].pin_index == hdmi_rst)
			continue;
#endif
		if (pin_type == PIN_INVALID) {
			printf("%s:pin[%d] is invalid\n", __func__, pin_info_array[i].pin_index);
			continue;
		}
		xj3_set_pin_dir(pin_info_array[i].pin_index, 0);
		xj3_set_gpio_pull(pin_info_array[i].pin_index,
				 pin_info_array[i].state,
				 pin_type);
		}
}

void xj3_set_pin(int pin, char *cmd)
{
	uint32_t pin_type = 0;
	if (strcmp(cmd, "f0") == 0) {
		xj3_set_pin_func(pin, 0);
	} else if (strcmp(cmd, "f1") == 0) {
		xj3_set_pin_func(pin, 1);
	} else if (strcmp(cmd, "f2") == 0) {
		xj3_set_pin_func(pin, 2);
	} else if (strcmp(cmd, "f3") == 0) {
		xj3_set_pin_func(pin, 3);
	} else if (strcmp(cmd, "ip") == 0) {
		xj3_set_pin_dir(pin, 0);
	} else if (strcmp(cmd, "op") == 0) {
		xj3_set_pin_dir(pin, 1);
	} else if (strcmp(cmd, "dh") == 0) {
		xj3_set_pin_value(pin, 1);
	} else if (strcmp(cmd, "dl") == 0) {
		xj3_set_pin_value(pin, 0);
	} else if (strcmp(cmd, "pu") == 0) {
		pin_type = xj3_pin_type(pin);
		xj3_set_gpio_pull(pin, PULL_UP, pin_type);
	} else if (strcmp(cmd, "pd") == 0) {
		pin_type = xj3_pin_type(pin);
		xj3_set_gpio_pull(pin, PULL_DOWN, pin_type);
	} else if (strcmp(cmd, "pn") == 0 || strcmp(cmd, "np") == 0) {
		pin_type = xj3_pin_type(pin);
		xj3_set_gpio_pull(pin, NO_PULL, pin_type);
	}
}

int xj3_set_pin_by_config(char *cfg_file, ulong file_addr)
{
	char *data, *dp, *lp, *ptr;
	loff_t size = 0;
	char line[128], filter_name[32];
	int length;
	int pin, start_pin, end_pin;
	int som_type = 0;

	// read gpio info from /boot/config.txt
	ptr = map_sysmem(file_addr, 0);
	if(hb_ext4_load(cfg_file, file_addr, &size) != 0)
	{
		printf("can't load config file(%s)\n", cfg_file);
		return 0;
	}
	if ((data = malloc(size + 1)) == NULL)
	{
		printf("can't malloc %lu bytes\n", (ulong)size + 1);
		return 0;
	}

	memcpy(data, ptr, size);
	data[size] = '\0';
	dp = data;

	// parse gpio info
	while ((length = hb_getline(line, sizeof(line), &dp)) > 0) {

		if (length == -1) {
			break;
		}

		lp = line;

		/* skip leading white space */
		while (isblank(*lp))
			++lp;

		/* skip comment lines */
		if (*lp == '#' || *lp == '\0')
			continue;

		/* Skip items that do not match the filter */
		if (som_type != 0 && som_type != hb_som_type_get())
			continue;

		if (strncmp(lp, "gpio=", 5) == 0)
		{
			lp += strlen("gpio=");
			char *str = strdup(lp);
			char *token;

			// Split by '=' to get key and value
			token = strtok(str, "=");
			if (token) {
				char *key = token;
				char *value = strtok(NULL, "=");

				char *dash = strchr(key, '-');
				if (dash) {
					// Handle ranges
					start_pin = atoi(key);
					end_pin = atoi(dash + 1);
				}
				else {
					start_pin = end_pin = atoi(key);
				}

				char *rangeToken = strtok(value, ",");
				while (rangeToken) {
					// set pin
					for (pin = start_pin; pin <= end_pin; pin++) {
						xj3_set_pin(pin, strim(rangeToken));
					}
					rangeToken = strtok(NULL, ",");
				}
			}

			free(str);
		}
		else if (strncmp(lp, "[", 1) == 0)
		{
			hb_extract_filter_name(lp, filter_name);
			som_type = hb_get_som_type_by_filter_name(filter_name);
			if (som_type == -1)
			{
				printf("Filter '%s' not found\n", filter_name);
				som_type = 0;
			}
		}
	}

	free(data);
	return 0;
}


static int do_set_pin_by_config(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{
	char *cfg_file;
	ulong file_addr;

	if (argc < 2)
		return CMD_RET_USAGE;

	cfg_file = argv[1];

	file_addr = simple_strtoul(argv[2], NULL, 16);

	return xj3_set_pin_by_config(cfg_file, file_addr);
}

static char set_pin_help_text[] =
	"setpin <config file> <config addr>  - set pin stat by /boot/config.txt\n";

U_BOOT_CMD(
	setpin, 255, 0, do_set_pin_by_config,
	"set pin by config", set_pin_help_text);

int xj3_set_pin_voltage_domain(char *cfg_file, ulong file_addr)
{
	char *data, *dp, *lp, *ptr;
	loff_t size = 0;
	char line[128], filter_name[32];
	int length;
	int som_type = 0;

	ptr = map_sysmem(file_addr, 0);
	if(hb_ext4_load(cfg_file, file_addr, &size) != 0)
	{
		printf("can't load config file(%s)\n", cfg_file);
		return 0;
	}
	if ((data = malloc(size + 1)) == NULL)
	{
		printf("can't malloc %lu bytes\n", (ulong)size + 1);
		return 0;
	}

	memcpy(data, ptr, size);
	data[size] = '\0';
	dp = data;

	while ((length = hb_getline(line, sizeof(line), &dp)) > 0) {
		if (length == -1) {
			break;
		}

		lp = line;

		/* skip leading white space */
		while (isblank(*lp))
			++lp;

		/* skip comment lines */
		if (*lp == '#' || *lp == '\0')
			continue;

		/* Skip items that do not match the filter */
		if (som_type != 0 && som_type != hb_som_type_get()) {
			continue;
		}

		if (strncmp(lp, "voltage_domain=", strlen("voltage_domain=")) == 0)
		{
			lp += strlen("voltage_domain=");

			/*
			* 1'b0=3.3v mode;  1'b1=1.8v mode
			* 0x170 bit[3]       sd2
			*       bit[2]       sd1
			*       bit[1:0]     sd0
			*
			* 0x174 bit[11:10]   rgmii
			*       bit[9]       i2c2
			*       bit[8]       i2c0
			*       bit[7]       reserved
			*       bit[6:4]     bt1120
			*       bit[3:2]     bifsd
			*       bit[1]       bifspi
			*       bit[0]       jtag
			*/
			printf("Set the voltage domain of 40pin to %s\n", lp);
			if (strncasecmp(lp, "3.3v", 4) == 0)
			{
				writel(0xC00, GPIO_BASE + 0x174);
			}
			else if (strncasecmp(lp, "1.8v", 4) == 0)
			{
				writel(0xF0F, GPIO_BASE + 0x174);
			}
		}
		else if (strncmp(lp, "[", 1) == 0)
		{
			hb_extract_filter_name(lp, filter_name);
			som_type = hb_get_som_type_by_filter_name(filter_name);
			if (som_type == -1)
			{
				printf("Filter '%s' not found\n", filter_name);
				som_type = 0;
			}
		}
	}

	free(data);
	return 0;
}

static int do_set_voltage_domain(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{
	char *cfg_file;
	ulong file_addr;

	if (argc < 2)
		return CMD_RET_USAGE;

	cfg_file = argv[1];

	file_addr = simple_strtoul(argv[2], NULL, 16);

	return xj3_set_pin_voltage_domain(cfg_file, file_addr);
}

U_BOOT_CMD(
	setvol, 255, 0, do_set_voltage_domain,
	"set pin voltage_domain",
	"setpin <config file> <config addr>  - set pin voltage_domain by /boot/config.txt");