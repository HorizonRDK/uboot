/*
 * Copyright (C) 2018/04/27 Horizon Robotics Co., Ltd.
 *
 * hb_pwm.h
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef __HB_PWM__
#define __HB_PWM__

/*************************************************************
 * base register
*************************************************************/

#define HB_PWM_BASE_ADDR_0			(0xA500D000)
#define HB_PWM_REG_0(offset)		(HB_PWM_BASE_ADDR_0 | offset)
#define HB_PWM_BASE_ADDR_1			(0xA500E000)
#define HB_PWM_REG_1(offset)		(HB_PWM_BASE_ADDR_1 | offset)
#define HB_PWM_BASE_ADDR_2			(0xA500F000)
#define HB_PWM_REG_2(offset)		(HB_PWM_BASE_ADDR_2 | offset)


/*************************************************************
 * register list
*************************************************************/

#define HB_PWM_PWM_EN				(0x0)
#define HB_PWM_TIME_SLICE			(0x4)
#define HB_PWM_PWM_FREQ				(0x8)
#define HB_PWM_PWM_FREQ1			(0xC)
#define HB_PWM_PWM_RATIO			(0x14)
#define HB_PWM_PWM_SRCPND			(0x1C)
#define HB_PWM_PWM_INTMASK			(0x20)
#define HB_PWM_PWM_SETMASK			(0x24)
#define HB_PWM_PWM_UNMASK			(0x28)


/*************************************************************
 * register bit
*************************************************************/

/* HB_PWM_PWM_EN */
#define HB_PWM_MODE_SEL(n)				(((n) & 0x1) << 0x6)
#define HB_PWM_MODE_SEL_MASK			(0x1 << 0x6)
#define HB_PWM_MODE_SEL_SHIT(n)			(((n) & 0x1) >> 0x6)
#define HB_PWM_PWM_INT_EN(n)			(((n) & 0x1) << 0x3)
#define HB_PWM_PWM_INT_EN_MASK			(0x1 << 0x3)
#define HB_PWM_PWM_INT_EN_SHIT(n)		(((n) & 0x1) >> 0x3)
#define HB_PWM_PWM2_EN(n)				(((n) & 0x1) << 0x2)
#define HB_PWM_PWM2_EN_MASK				(0x1 << 0x2)
#define HB_PWM_PWM2_EN_SHIT(n)			(((n) & 0x1) >> 0x2)
#define HB_PWM_PWM1_EN(n)				(((n) & 0x1) << 0x1)
#define HB_PWM_PWM1_EN_MASK				(0x1 << 0x1)
#define HB_PWM_PWM1_EN_SHIT(n)			(((n) & 0x1) >> 0x1)
#define HB_PWM_PWM0_EN(n)				(((n) & 0x1) << 0x0)
#define HB_PWM_PWM0_EN_MASK				(0x1 << 0x0)
#define HB_PWM_PWM0_EN_SHIT(n)			(((n) & 0x1) >> 0x0)

/* HB_PWM_TIME_SLICE */
#define HB_PWM_REG_TIME_SLICE(n)		(((n) & 0xffff) << 0x0)
#define HB_PWM_REG_TIME_SLICE_MASK		(0xffff << 0x0)
#define HB_PWM_REG_TIME_SLICE_SHIT(n)	(((n) & 0xffff) >> 0x0)

/* HB_PWM_PWM_FREQ */
#define HB_PWM_PWM_FREQ(n)					(((n) & 0xfff) << 0x10)
#define HB_PWM_PWM_FREQ_MASK				(0xfff << 0x10)
#define HB_PWM_PWM_FREQ_SHIT(n)				(((n) & 0xfff) >> 0x10)
#define HB_PWM_PWM_FREQ0(n)					(((n) & 0xfff) << 0x0)
#define HB_PWM_PWM_FREQ0_MASK				(0xfff << 0x0)
#define HB_PWM_PWM_FREQ0_SHIT(n)			(((n) & 0xfff) >> 0x0)

/* HB_PWM_PWM_FREQ1 */
#define HB_PWM_PWM_FREQ2(n)				(((n) & 0xfff) << 0x0)
#define HB_PWM_PWM_FREQ2_MASK			(0xfff << 0x0)
#define HB_PWM_PWM_FREQ2_SHIT(n)		(((n) & 0xfff) >> 0x0)

/* HB_PWM_PWM_RATIO */
#define HB_PWM_PWM_RATIO(n)				(((n) & 0xff) << 0x10)
#define HB_PWM_PWM_RATIO_MASK			(0xff << 0x10)
#define HB_PWM_PWM_RATIO_SHIT(n)		(((n) & 0xff) >> 0x10)
#define HB_PWM_PWM_RATIO1(n)			(((n) & 0xff) << 0x8)
#define HB_PWM_PWM_RATIO1_MASK			(0xff << 0x8)
#define HB_PWM_PWM_RATIO1_SHIT(n)		(((n) & 0xff) >> 0x8)
#define HB_PWM_PWM_RATIO0(n)			(((n) & 0xff) << 0x0)
#define HB_PWM_PWM_RATIO0_MASK			(0xff << 0x0)
#define HB_PWM_PWM_RATIO0_SHIT(n)		(((n) & 0xff) >> 0x0)

/* HB_PWM_PWM_SRCPND */
#define HB_PWM_PWM_SRCPND_W1C			(0x1 << 0x0)

/* HB_PWM_PWM_INTMASK */
#define HB_PWM_PWM_INTMASK_RO			(0x1 << 0x0)
#define HB_PWM_PWM_INTMASK_RO_SHIT(n)	(((n) & 0x1) >> 0x0)

/* HB_PWM_PWM_SETMASK */
#define HB_PWM_PWM_SETMASK_WO(n)		(((n) & 0x1) << 0x0)

/* HB_PWM_PWM_UNMASK */
#define HB_PWM_PWM_UNMASK_WO(n)			(((n) & 0x1) << 0x0)

#endif /*__HB_PWM__*/

