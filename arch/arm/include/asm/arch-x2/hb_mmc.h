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
/*         04 02 2018   shaochuanzhang@hobot.cc  Draft                                */
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
#define PIN_MUX_BASE    0xA6003000
#define GPIO3_CFG (PIN_MUX_BASE + 0x30)
#define GPIO3_DIR (PIN_MUX_BASE + 0x38)
#define GPIO3_VAL (PIN_MUX_BASE + 0x3C)

#define GPIO5_CFG (PIN_MUX_BASE + 0x50)
#define GPIO5_DIR (PIN_MUX_BASE + 0x58)
#define GPIO5_VAL (PIN_MUX_BASE + 0x5C)

#define GPIO7_CFG (PIN_MUX_BASE + 0x70)
#define GPIO7_DIR (PIN_MUX_BASE + 0x78)
#define GPIO7_VAL (PIN_MUX_BASE + 0x7C)

/* x2 MMC AHB related Registers */
#define HB_MMC_BASE 0xA5010000
#define MMC_IRQ_NUM (71)
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
static inline void sdio_reset(void)
{
    writel(0x8000, SYSCTRL_BASE + 0x450);
    udelay(100);
    writel(0, SYSCTRL_BASE + 0x450);
}
#endif /* __HB_MMC_H__ */

