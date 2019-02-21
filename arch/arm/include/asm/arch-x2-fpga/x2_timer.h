#ifndef __ASM_ARCH_X2_TIMER_H__
#define __ASM_ARCH_X2_TIMER_H__

#include <asm/arch/x2_reg.h>

#define TIMER_CTRL_STA_REG	(TIMER0_BASE + 0x00)
#define TIMER_START_REG		(TIMER0_BASE + 0x04)
#define TIMER_STOP_REG		(TIMER0_BASE + 0x08)
#define TIMER_MODE_REG		(TIMER0_BASE + 0x0C)

#define TIMER_SUB1_TARGET_REG	(TIMER0_BASE + 0x20)
#define TIMER_SUB1_CUR_REG		(TIMER0_BASE + 0x24)

#define TIMER_WDTGT_REG		(TIMER0_BASE + 0x28)
#define TIMER_WDWAIT_REG	(TIMER0_BASE + 0x2C)
#define TIMER_WDCLR_REG		(TIMER0_BASE + 0x38)

/* TIMER_CTRL_STA_REG */
#define TM_SUB0_STA(x)		(((x) >> 0) & 0x1)
#define TM_SUB1_STA(x)		(((x) >> 1) & 0x1)
#define TM_SUB2_STA(x)		(((x) >> 2) & 0x1)

/* TIMER_START_REG */
#define TM_SUB0_START()		(1 << 0)
#define TM_SUB1_START()		(1 << 1)
#define TM_SUB2_START()		(1 << 2)

/* TIMER_STOP_REG */
#define TM_SUB0_STOP()		(1 << 0)
#define TM_SUB1_STOP()		(1 << 1)
#define TM_SUB2_STOP()		(1 << 2)

/* TIMER_MODE_REG */
#define TM_SUB0_MODE(x)		(((x) & 0xF) << 0)
#define TM_SUB1_MODE(x)		(((x) & 0xF) << 4)
#define TM_SUB2_MODE(x)		(((x) & 0xF) << 8)

/* TIMER_SUB1_TARGET_REG */
#define TM_SUB1_TARGET(x)	((x) & 0xFFFFFFFF)

/* TIMER_WDTGT_REG */
#define TM_SUB2_WDTGT(x)	((x) & 0xFFFFFFFF)

/* TIMER_WDWAIT_REG */
#define TM_SUB2_WDWAIT(x)	((x) & 0xFFFFFFFF)

void x2_timer_init(unsigned int value);

void x2_timer_enalbe(void);

void x2_timer_disable(void);

unsigned int x2_timer_get_val32(void);

#endif /* __ASM_ARCH_X2_TIMER_H__ */
