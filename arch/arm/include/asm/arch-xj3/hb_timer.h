/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#ifndef ARCH_ARM_INCLUDE_ASM_ARCH_X3_HB_TIMER_H_
#define ARCH_ARM_INCLUDE_ASM_ARCH_X3_HB_TIMER_H_

/* HB TIMER register offsets */
#define HB_TIMER_REG_BASE	0xA1002000

#define HB_TIMER_TMREN_REG		  0x00
#define HB_TIMER_TMRSTART_REG	  0x04
#define HB_TIMER_TMRSTOP_REG	  0x08
#define HB_TIMER_TMRMODE_REG	  0x0C
#define HB_TIMER_TMR0TGTL_REG	  0x10
#define HB_TIMER_TMR0TGTH_REG	  0x14
#define HB_TIMER_TMR0DL_REG 	  0x18
#define HB_TIMER_TMR0DH_REG 	  0x1C
#define HB_TIMER_TMR1TGT_REG	  0x20
#define HB_TIMER_TMR1D_REG		  0x24
#define HB_TIMER_WDTGT_REG		  0x28
#define HB_TIMER_WDWAIT_REG 	  0x2C
#define HB_TIMER_WD1D_REG		  0x30
#define HB_TIMER_WD2D_REG		  0x34
#define HB_TIMER_WDCLR_REG		  0x38
#define HB_TIMER_TMR_SRCPND_REG   0x3C
#define HB_TIMER_TMR_INTMASK_REG  0x40
#define HB_TIMER_TMR_SETMASK_REG  0x44
#define HB_TIMER_TMR_UNMASK_REG   0x48

/* HB TIMER register op-bit Masks */
#define HB_TIMER_T0START		  BIT(0)
#define HB_TIMER_T1START		  BIT(1)
#define HB_TIMER_T2START		  BIT(2)
#define HB_TIMER_T0STOP 		  BIT(0)
#define HB_TIMER_T1STOP 		  BIT(1)
#define HB_TIMER_T2STOP 		  BIT(2)
#define HB_TIMER_ONE_MODE		  0x0	  /* one-time mode */
#define HB_TIMER_PRD_MODE		  0x1	  /* periodical mode */
#define HB_TIMER_CON_MODE		  0x2	  /* continuous mode */
#define HB_TIMER_WDT_MODE		  0x3	  /* watchdog mode */
#define HB_TIMER_T0MODE_OFFSET	  0x0
#define HB_TIMER_T1MODE_OFFSET	  0x4
#define HB_TIMER_T2MODE_OFFSET	  0x8
#define HB_TIMER_T0_INTMASK		  BIT(0)
#define HB_TIMER_T1_INTMASK		  BIT(1)
#define HB_TIMER_T2_INTMASK		  BIT(2)
#define HB_TIMER_WDT_INTMASK	  HB_TIMER_T2_INTMASK
#define HB_TIMER_WDT_RESET        BIT(0)
#define HB_TIMER_REF_CLOCK		  24000000

/* timeout value (in seconds) */
#define HB_WDT_NORMAL_BARK_TIMEOUT	(20 * (HB_TIMER_REF_CLOCK / 24))
#define HB_WDT_NORMAL_BITE_TIMEOUT	0x1000
#define HB_WDT_RESET_TIMEOUT	0x1000
#define HB_WDT_MIN_TIMEOUT		1

#endif // ARCH_ARM_INCLUDE_ASM_ARCH_X3_HB_TIMER_H_
