#include <asm-offsets.h>
#include <config.h>
#include <asm/system.h>
#include <asm/macro.h>
#include <linux/linkage.h>

.global g_binfo
.global g_bootsrc
.global g_romver

ENTRY(save_boot_params)
#ifdef CONFIG_SPL_BUILD
	ldr x3, =g_romver
	str w23, [x3]

	ldr x3, =g_bootsrc		/* Copy binfo to g_binfo */
	str x0, [x3]
	cbz x0, save_ret
	ldr x0, =g_binfo
copy_loop:
	ldp x4, x5, [x1], #16
	stp x4, x5, [x0], #16
	subs x2, x2, #16
	cbnz x2, copy_loop

#endif /* CONFIG_SPL_BUILD */

save_ret:
	ldr	x4, =0xA6000278
	str wzr, [x4]			/* PMU_SW_REG30 */
	str wzr, [x4, #0x4]		/* PMU_SW_REG31 */

	ldr	x5, =save_boot_params_ret
	cbz w23, hb_rel1
	str x5, [x4]			/* Update start-up addr in rel1_1 */
	sev
	b hb_ret
hb_rel1:
	ldr	x4, =0xA1000500		/* Warm rest in rel0 */
	str	w5, [x4, #0x4]		/* CORE1_RVBAR_REG */
	mov	w5, #0x10
	str	w5, [x4, #0x8]		/* CORE_RVBAR_SEL */
	str	w5, [x4, #0x30]		/* CORE_WRST_REQ */
hb_ret:
	b	save_boot_params_ret
ENDPROC(save_boot_params)

#ifdef CONFIG_SPL_BUILD
ENTRY(smc_hack)
	/*
	 * Update bifsd's irq to group1 and
	 * disable fiq feature of group0 on core0.
	 */
	ldr x0, =0x90001000
	mov w1, #~0
	str w1, [x0, #0x84]
	ldr x0, =0x90002000
	ldr x1, [x0]
	bic w1, w1, #(0x1 << 3)
	str w1, [x0]
	ldr x29, =0xffff43e8
	mov lr, x29
	ret
ENDPROC(smc_hack)
#endif /* CONFIG_SPL_BUILD */

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_HOBOT_BIFSD)
WEAK(hr_gic_irq_init)
    mov x29, lr            /* Save LR */

    msr DAIFSet, #0x8

    ldr x0, =GICD_BASE
    mov w1, #~0
    str w1, [x0, #0x104]

/* config interrupt priority */

	ldr x0, =GICD_BASE
    ldr w1, =GICD_PRIORITY_PARAM
    str w1,[x0, #0x420]
    str w1,[x0, #0x424]
    str w1,[x0, #0x428]
    str w1,[x0, #0x42C]
    str w1,[x0, #0x430]
    str w1,[x0, #0x434]
    str w1,[x0, #0x438]

/* config Target to Core0 */
    ldr x0, =GICD_BASE
    ldr w1, =GICD_TARGET_PARM

    str w1,[x0, #0x820]
    str w1,[x0, #0x824]
    str w1,[x0, #0x828]
    str w1,[x0, #0x82C]
    str w1,[x0, #0x830]
    str w1,[x0, #0x834]
    str w1,[x0, #0x838]
    str w1,[x0, #0x83C]
    str w1,[x0, #0x840]
    str w1,[x0, #0x844]
    str w1,[x0, #0x848]
    str w1,[x0, #0x84C]
    str w1,[x0, #0x850]
    str w1,[x0, #0x854]
    str w1,[x0, #0x858]
    str w1,[x0, #0x85C]

	msr SPSel, #1
    msr DAIFClr, #0x7

    mov lr, x29            /* Restore LR */
    ret
ENDPROC(hr_gic_irq_init)
#endif /* !CONFIG_SPL_BUILD */


ENTRY(lowlevel_init)
	mov	x29, lr			/* Save LR */

#ifdef CONFIG_SPL_BUILD

#ifdef CONFIG_ARMV8_MULTIENTRY
	branch_if_master x0, x1, 2f

	ldr x7, =0x90001000
	ldr x8, =0x90002000
	/*
	 * Initialize SGIs and PPIs
	 * x7: Distributor Base
	 * x8: Cpu Interface Base
	 */
	mov	w9, #~0			/* Config SGIs and PPIs as Grp1 */
	str	w9, [x7, 0x80]	/* GICD_IGROUPR0 */
	mov	w9, #0x1		/* Enable SGI 0 */
	str	w9, [x7, 0x100]	/* GICD_ISENABLERn */

	/* Initialize Cpu Interface */
	mov	w9, #0x1e7		/* Disable IRQ/FIQ Bypass & */
					/* Enable Ack Group1 Interrupt & */
					/* EnableGrp0 & EnableGrp1 */
	str	w9, [x8]	/* Secure GICC_CTLR */

	mov	w9, #0x1 << 7		/* Non-Secure access to GICC_PMR */
	str	w9, [x8, 0x4]	/* GICC_PMR */

	/*
	 * All slaves will enter EL2 and optionally EL1.
	 */
	adr	x4, lowlevel_in_el2
	ldr	x5, =ES_TO_AARCH64
	bl	armv8_switch_to_el2

lowlevel_in_el2:
#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
	adr	x4, lowlevel_in_el1
	ldr	x5, =ES_TO_AARCH64
	bl	armv8_switch_to_el1

lowlevel_in_el1:
#endif

#endif /* CONFIG_ARMV8_MULTIENTRY */

#endif /* CONFIG_SPL_BUILD */

2:
#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_HOBOT_BIFSD)
	bl  hr_gic_irq_init
#endif /* !CONFIG_SPL_BUILD */
	mov	lr, x29			/* Restore LR */
	ret
ENDPROC(lowlevel_init)
