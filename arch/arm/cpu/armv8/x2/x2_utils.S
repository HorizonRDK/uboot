#include <asm-offsets.h>
#include <config.h>
#include <asm/system.h>
#include <asm/macro.h>
#include <linux/linkage.h>

.global g_binfo
.global g_bootsrc

ENTRY(save_boot_params)
#ifdef CONFIG_SPL_BUILD
	ldr x3, =g_bootsrc
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
	b	save_boot_params_ret
ENDPROC(save_boot_params)

/* Update bifsd's irq to group1 and disable fiq feature of group0. */
#ifdef CONFIG_SPL_BUILD
ENTRY(smc_hack)
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
