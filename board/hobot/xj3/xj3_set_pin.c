/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#include <common.h>
#include <sata.h>
#include <asm/io.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_sysctrl.h>
#include <hb_info.h>
#include <asm/arch-x2/ddr.h>
#include "xj3_set_pin.h"

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

static void xj3_set_pin_dir(char pin)
{
	uint32_t reg = 0;
	uint32_t offset = 0;

	/* set pin to input */
	offset = (pin / 16) * 0x10 + 0x08;
	reg = reg32_read(X2_GPIO_BASE + offset);
	reg &= (~(1 << ((pin % 16) + 16)));
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

void xj3_set_pin_info(void)
{
	int32_t i = 0;
	uint32_t pin_type = 0;
	for (i = 0; i < pin_info_len; i++) {
		pin_type = xj3_pin_type(pin_info_array[i].pin_index);
		if (pin_type == PIN_INVALID) {
			printf("%s:pin[%d] is invalid\n", __func__, pin_info_array[i].pin_index);
			continue;
		}
		xj3_set_pin_dir(pin_info_array[i].pin_index);
		xj3_set_gpio_pull(pin_info_array[i].pin_index,
				 pin_info_array[i].state,
				 pin_type);
		}
}
