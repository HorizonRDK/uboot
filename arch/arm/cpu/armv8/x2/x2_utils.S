#include <asm-offsets.h>
#include <config.h>
#include <asm/system.h>
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
