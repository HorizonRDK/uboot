#ifndef __BOOT0_H__
#define __BOOT0_H__

	ldr x21, =0xA6000274
	ldr w23, [x21]
	cbnz w23, skip_smc
#ifdef CONFIG_SPL_BUILD
.globl smc_hack

	mov	x21, x0
	mov	x22, x1
	mov	x0, #0x12
	ldr	x1, =smc_hack
	smc	#0
	mov	x0, x21
	mov	x1, x22
#endif /* CONFIG_SPL_BUILD */

skip_smc:

#endif
