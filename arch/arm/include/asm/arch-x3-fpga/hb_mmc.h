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
#define SD0_PIN_MUX_BASE    0xA6004000
#define SD0_CLK         (SD0_PIN_MUX_BASE + 0xD4)
#define SD0_CMD         (SD0_PIN_MUX_BASE + 0xD8)
#define SD0_DATA0       (SD0_PIN_MUX_BASE + 0xDC)
#define SD0_DATA1       (SD0_PIN_MUX_BASE + 0xE0)
#define SD0_DATA2       (SD0_PIN_MUX_BASE + 0xE4)
#define SD0_DATA3       (SD0_PIN_MUX_BASE + 0xE8)
#define SD0_DATA4       (SD0_PIN_MUX_BASE + 0xEC)
#define SD0_DATA5       (SD0_PIN_MUX_BASE + 0xF0)
#define SD0_DATA6       (SD0_PIN_MUX_BASE + 0xF4)
#define SD0_DATA7       (SD0_PIN_MUX_BASE + 0xF8)
#define SD0_DATA_STRB   (SD0_PIN_MUX_BASE + 0xFC)
#define SD0_DET_N       (SD0_PIN_MUX_BASE + 0x100)
#define SD0_WPROT       (SD0_PIN_MUX_BASE + 0x104)



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

