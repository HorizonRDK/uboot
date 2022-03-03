/*
 * (C) Copyright 2021
 * Horizon Robotics, Inc.
 */
#include <common.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <asm/io.h>
#include <cpu.h>
#include <initcall.h>
#include <hb_info.h>
#include <linux/arm-smccc.h>
#include <asm/psci.h>
#include "configs/xj3_cpus.h"

DECLARE_GLOBAL_DATA_PTR;

/*	global data location
** +-----------------+ High addr
** | 	 core1	sp   |
** +-----------------+
** | 	 core1  gd   |
** +-----------------+
** | 	 core0	gd   |
** +-----------------+ Low addr
*/
uint64_t gd_slave_core1 = 0; /*core1 gd addr*/
uint64_t gd_master_core = 0; /*core0 gd addr*/
uint64_t sp_slave_core1 = 0;
/*point to slave core malloc base, get vaule from core0*/
void *core1_malloc_base = NULL;

extern void _start(void);
extern void slave_e(void);
extern void psci_cpu_entry(void);
extern int initr_net(void);/*init eth phy*/

volatile wait_salve_core_t wait_salve_core = NULL;
static struct reserve_mem* r_mem = NULL;

static void __wait_salve_core(void);
static inline uint64_t __attribute_const__ read_cpuid_mpidr(void);

static uint64_t __invoke_psci_fn_smc(uint64_t function_id, uint64_t arg0,
				     uint64_t arg1, uint64_t arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}
/**
 * kill slave cpu core
 * this function can auto detect who want kill slave cores.
*/
uint64_t hb_kill_slave_core(void)
{
	uint64_t cpu_id;
	volatile unsigned int* done_flag = &r_mem->done;
	cpu_id = read_cpuid_mpidr();

	*done_flag = SLAVE_CORE_DONE;
	if (cpu_id == CPU_CORE_0) {
		DEBUG_LOG("core0: kill slave core!\n");
		if (core1_malloc_base != NULL) {
			free(core1_malloc_base);
			core1_malloc_base = NULL;
		}
		return __invoke_psci_fn_smc(ARM_PSCI_0_2_FN_AFFINITY_INFO,
					    SLAVE_CORE_ID, 0, 0);
	} else if (cpu_id == CPU_CORE_1) {
		return __invoke_psci_fn_smc(ARM_PSCI_0_2_FN_CPU_OFF,
					    SLAVE_CORE_SMCC_ARGS, 0, 0);
	} else {
		pr_err("kill_slave_core_fail,unkonow cpu cores:0x%llx", cpu_id);
		return 1;
	}
}

/*Wake up the slave core by the master core*/
int wake_slave_core(void)
{
	uint64_t __maybe_unused ret;

	uintptr_t wakeup_entry = (uintptr_t)psci_cpu_entry;
	wait_salve_core = __wait_salve_core;
	writel(0, CPU_RELEASE_ADDR);

	DEBUG_LOG("wake up cpu: core%d\n", SLAVE_CORE_ID);
	debug("### gd = 0x%p, gd->malloc_base = 0x%lx, gd size = 0x%lx\n",
		  gd, gd->malloc_base, sizeof(struct global_data));
	ret = __invoke_psci_fn_smc(ARM_PSCI_0_2_FN_CPU_ON,
							   SLAVE_CORE_ID, (uint64_t)slave_e, 0);
    gd_master_core = (uint64_t)gd;
	gd_slave_core1 = (uint64_t)gd + sizeof(struct global_data);
	sp_slave_core1 = (uint64_t)gd + sizeof(struct global_data)
			 * 2 + SLAVE_CORE_STACK_SIZE;

	sp_slave_core1 = sp_slave_core1 - sizeof(struct reserve_mem);
	r_mem = (struct reserve_mem*)sp_slave_core1;
	*((volatile unsigned int*)(&r_mem->done)) = 0;

	debug("### gd_slave_core1 = 0x%llx, sp_slave_core1 = 0x%llx, &done = 0x%p\n",
		  gd_slave_core1, sp_slave_core1, &r_mem->done);
	flush_dcache_all();
	writel((uint32_t)wakeup_entry, CPU_RELEASE_ADDR);
	isb();
	asm volatile("sev");

	__invoke_psci_fn_smc(ARM_PSCI_0_2_FN_AFFINITY_INFO, 1, 0, 0);
	debug("### %s return, ret = %llu\n", __func__, ret);
	return 0;
}

static void __wait_salve_core(void)
{
	volatile unsigned int* done_flag = &r_mem->done;
	debug("### master core start waiting the salve core%d\n", SLAVE_CORE_ID);
	DEBUG_LOG("waiting for salve cpu: core%d complete...", SLAVE_CORE_ID);
	invalidate_dcache_all();
	while(*done_flag != SLAVE_CORE_DONE) {
#ifdef SLAVE_CORE_DEBUG
		unsigned int cnt = 0;
		cnt++;
		if ((cnt % SPAIN_PRINT_CNT) == 0)
			debug("### master core waiting salve core count %u, done_flag 0x%x\n",
				  cnt, *done_flag);
#endif /*SLAVE_CORE_DEBUG*/
		udelay(SPAIN_DELAY_TIME);
		invalidate_dcache_all();
	}
	udelay(500);
	__invoke_psci_fn_smc(ARM_PSCI_0_2_FN_AFFINITY_INFO, SLAVE_CORE_ID, 0, 0);
	DEBUG_LOG("OK\n");
	debug("### salve core%d power off, done_flag = 0x%x\n",
		  SLAVE_CORE_ID, *done_flag);
}

static int init_slave_core_mem(void)
{
	/*First: copy core0 global data*/
	debug(
		"core1 start, core0 gd addr=0x%llx,sizeof(gd)=0x%lx,core1 gd addr=0x%llx\n",
		gd_master_core, sizeof(struct global_data), gd_slave_core1);
	memcpy((void *)gd_slave_core1, (void *)gd_master_core,
	       sizeof(struct global_data));
	show_gd_info("core1 start run", gd);

	/*Second: Wait for core0 malloc free mem*/
	while (NULL == core1_malloc_base) {
		udelay(100);
		DEBUG_LOG("wait for core1_malloc_base\n");
	}
	gd->malloc_base = (uint64_t)core1_malloc_base;
	gd->malloc_ptr = 0;
	gd->malloc_limit = SLAVE_CORE_MALLOC_SIZE_MAX;
	debug("slave_core: malloc_base=0x%lx,malloc_limit=0x%lx\n",
	      gd->malloc_base, gd->malloc_limit);
	return 0;
}

static inline uint64_t __attribute_const__ read_cpuid_mpidr(void)
{
	uint64_t __val;
	asm volatile("mrs %0, mpidr_el1" : "=r" (__val));
	return __val;
}

static void slave_core1_work_list(void)
{
	init_slave_core_mem();
	/*Place the function you need the slave CPU core to do after here*/

}

void slave_core1_cycle(void)
{
	volatile unsigned int* done_flag = &r_mem->done;
	slave_core_board_init();
	enable_caches();
	debug("slave cpu core%d start, mpidr_el1 = 0x%llx\n",
			SLAVE_CORE_ID, read_cpuid_mpidr());

	slave_core1_work_list();
	*done_flag = SLAVE_CORE_DONE;
	isb();

	debug("slave cpu core%d psci off, done_flag 0x%x, mpidr_el1 = 0x%llx\n",
			  SLAVE_CORE_ID, *done_flag, read_cpuid_mpidr());
	/*don't care 65536*/
	__invoke_psci_fn_smc(ARM_PSCI_0_2_FN_CPU_OFF, SLAVE_CORE_SMCC_ARGS, 0, 0);
	debug("### core%d work done, mpidr_el1 = 0x%llx\n",
		  SLAVE_CORE_ID, read_cpuid_mpidr());
	while (1)
		wfi();
}

void show_gd_info(const char *s, const volatile struct global_data *p_gd)
{
	debug("###%s  .", s);
	debug("gd:0x%p,", p_gd);
	debug("flags=0x%lx,", p_gd->flags);
	debug("have_console=%ld,", p_gd->have_console);
	debug("irq_sp=0x%lx,", p_gd->irq_sp);
	debug("ram_top=0x%lx,", p_gd->ram_top);
	debug("ram_base=0x%lx,", p_gd->ram_base);
	debug("ram_size=0x%llx,", p_gd->ram_size);
	debug("fdt_size=0x%lx,", p_gd->fdt_size);
	debug("malloc_base=0x%lx,", p_gd->malloc_base);
	debug("malloc_limit=0x%lx,", p_gd->malloc_limit);
	debug("malloc_ptr=0x%lx,", p_gd->malloc_ptr);
	debug("fdt_blob=0x%p,", p_gd->fdt_blob);
	debug("\n");
}
