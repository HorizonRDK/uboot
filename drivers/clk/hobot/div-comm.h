/*
 *  /drivers/clk/hobot/div_comm.h
 *
 *  This head file can be  used for div-gate.c and endiv_gate.c.
 *  In HOBOT clock driver, div-gate.c and endiv_gate.c have common interfaces to use.
*/

#ifndef DRIVERS_CLK_HOBOT_DIV_COMM_H_
#define DRIVERS_CLK_HOBOT_DIV_COMM_H_

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>

#include "clk-common.h"

#define div_mask(width)	((1 << (width)) - 1)

struct div_platdata {
	phys_addr_t divider_reg;
	unsigned int div_bits;
	unsigned int div_field;
};

struct gate_platdata {
	phys_addr_t clken_sta_reg;
	uint clken_sta_bit;
	uint clken_sta_field;
	phys_addr_t enable_reg;
	uint enable_bit;
	uint enable_field;
	phys_addr_t disable_reg;
	uint disable_bit;
	uint disable_field;
};

#endif // DRIVERS_CLK_HOBOT_DIV_COMM_H_

