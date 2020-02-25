/*****************************************************************************/
/*                                                                           */
/*                     HozironRobtics MMC MAC SOFTWARE                       */
/*                                                                           */
/*                  Horizon Robtics SYSTEMS LTD                              */
/*                           COPYRIGHT(C) 2018                               */
/*                                                                           */
/*  This program  is  proprietary to  Ittiam  Systems  Private  Limited  and */
/*  is protected under china Copyright Law as an unpublished work. Its use   */
/*  and  disclosure  is  limited by  the terms  and  conditions of a license */
/*  agreement. It may not be copied or otherwise  reproduced or disclosed to */
/*  persons outside the licensee's organization except in accordance with the*/
/*  terms  and  conditions   of  such  an  agreement.  All  copies  and      */
/*  reproductions shall be the property of HorizonRobtics Systems Private    */
/*    Limited and must bear this notice in its entirety.                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  File Name         : hb_mmc.h                                        */
/*                                                                           */
/*  Description       : This file contains all the declarations related to   */
/*                      MMC host interface.                                  */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         04 02 2018   shaochuanzhang@hobot.cc  Draft                       */
/*                                                                           */
/*****************************************************************************/

#ifndef __HB_MMC_H__
#define __HB_MMC_H__

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include <asm/io.h>
#include <asm/arch/hb_reg.h>

/*****************************************************************************/
/* Constants                                                                 */
/*****************************************************************************/

/* GPIO PIN MUX */
#define SD0_CLK         (PIN_MUX_BASE + 0xD4)
#define SD0_CMD         (PIN_MUX_BASE + 0xD8)
#define SD0_DATA0       (PIN_MUX_BASE + 0xDC)
#define SD0_DATA1       (PIN_MUX_BASE + 0xE0)
#define SD0_DATA2       (PIN_MUX_BASE + 0xE4)
#define SD0_DATA3       (PIN_MUX_BASE + 0xE8)
#define SD0_DATA4       (PIN_MUX_BASE + 0xEC)
#define SD0_DATA5       (PIN_MUX_BASE + 0xF0)
#define SD0_DATA6       (PIN_MUX_BASE + 0xF4)
#define SD0_DATA7       (PIN_MUX_BASE + 0xF8)
#define SD0_DATA_STRB   (PIN_MUX_BASE + 0xFC)
#define SD0_DET_N       (PIN_MUX_BASE + 0x100)
#define SD0_WPROT       (PIN_MUX_BASE + 0x104)

#define SD1_CLK         (PIN_MUX_BASE + 0x108)
#define SD1_CMD         (PIN_MUX_BASE + 0x10C)
#define SD1_DATA0       (PIN_MUX_BASE + 0x110)
#define SD1_DATA1       (PIN_MUX_BASE + 0x114)
#define SD1_DATA2       (PIN_MUX_BASE + 0x118)
#define SD1_DATA3       (PIN_MUX_BASE + 0x11C)

#define SD2_CLK         (PIN_MUX_BASE + 0x120)
#define SD2_CMD         (PIN_MUX_BASE + 0x124)
#define SD2_DATA0       (PIN_MUX_BASE + 0x128)
#define SD2_DATA1       (PIN_MUX_BASE + 0x12C)
#define SD2_DATA2       (PIN_MUX_BASE + 0x130)
#define SD2_DATA3       (PIN_MUX_BASE + 0x134)

#define SD0_RSTN        BIT(16)
#define SD1_RSTN        BIT(17)
#define SD2_RSTN        BIT(18)

/* j3 sd card power on ctrl EN_VDD_CNN0/GPIO0[1], output 0 enable */
#define SD1_POWER_PIN_MUX	(PIN_MUX_BASE + 0x04)
#define SD1_POWER_OUTPUT_CTRL	(GPIO_BASE + 0x08)
#define SD1_POWER_DIR		BIT(17)
#define SD1_POWER_OUTPUT	BIT(1)

/* x3 MMC AHB related Registers */
#define MMC_IRQ_NUM	(79)
#define SDIO_INT_MODE_ENABLE

/*****************************************************************************/
/* Data Types                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Extern variable declarations                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/
extern void hb_mmc_isr(void);
/*****************************************************************************/
/* INLINE Functions                                                          */
/*****************************************************************************/
#endif /* __HB_MMC_H__ */
