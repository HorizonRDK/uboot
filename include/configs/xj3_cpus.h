/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#ifndef INCLUDE_CONFIGS_XJ3_CPUS_H_
#define INCLUDE_CONFIGS_XJ3_CPUS_H_

// #define SP_DEBUG
#ifdef SP_DEBUG
#define READ_SP()  ({                           \
	uint64_t __sp;	                            \
	asm volatile("mov %0, sp" : "=r" (__sp));   \
	__sp;                                       \
})
#define SP_DBG()                                 \
	do {                                         \
		static uint64_t rec_sp = ((uint64_t)-1); \
		uint64_t __sp_tmp = READ_SP();           \
		if (__sp_tmp < rec_sp) {                 \
			rec_sp = __sp_tmp;                   \
			printf("%s[%d]: sp = 0x%llx\n",      \
				   __func__, __LINE__, rec_sp);  \
		}                                        \
	} while (0)
#else
#define SP_DBG()
#endif /* SP_DEBUG */

#define RESERVE_MEM_LEN    (64)
struct reserve_mem{
	char reserve[RESERVE_MEM_LEN - sizeof(uint32_t)];
	uint32_t done;
};

#define CPU_NUM                             (4)
#define COR0_PWR_CTRL_REG                   (0xA1000490UL)
#define COR1_PWR_CTRL_REG                   (0xA1000494UL)
#define COR2_PWR_CTRL_REG                   (0xA1000498UL)
#define COR3_PWR_CTRL_REG                   (0xA100049CUL)
#define CORn_PWR_CTRL_REG_PWR_ON            (0x0001)
#define CORn_PWR_CTRL_REG_STS               (0x0F00)
#define CORn_PWR_CTRL_REG_STS_COMPLETE      (0x0800)

#define PMU_CPU_WFI_STS                     (0xA6000048)
#define X2A_CPU1_POWEROFF                   (0xA6000240)
#define X2A_CPU2_POWEROFF                   (0xA6000244)
#define X2A_CPU3_POWEROFF                   (0xA6000248)
#define CPU_CORE_ONE_RELEASE_ADDR		    X2A_CPU1_POWEROFF

#define SLAVE_CORE_ID     (1)
#define SLAVE_CORE_SMCC_ARGS (0x10000)
#define SLAVE_CORE_DONE   (0xA5A5A5A5)
#define SLAVE_CORE_STACK_SIZE  (1024 * 64)   // 64K
#define SLAVE_CORE_MALLOC_SIZE_MAX		\
	((CONFIG_VAL(SYS_MALLOC_F_LEN)) / 4) // core1 malloc limit: 4K

#define SPAIN_DELAY_TIME      (10)
#define SPAIN_PRINT_CNT       (500000 / SPAIN_DELAY_TIME)

#define ERR_NO_FREE_MEM			1
#define ERR_ENV_NOT_READY		2
#define ERR_UNKNOW_CPU			3

typedef void(*wait_salve_core_t)(void);
extern volatile wait_salve_core_t wait_salve_core;
extern void *core1_malloc_base;

#define CPU_CORE_MPIDR_EL1_BASE (0x80000000)
#define CPU_CORE_0 (CPU_CORE_MPIDR_EL1_BASE + 0x0)
#define CPU_CORE_1 (CPU_CORE_MPIDR_EL1_BASE + 0x1)
#define CPU_CORE_2 (CPU_CORE_MPIDR_EL1_BASE + 0x2)
#define CPU_CORE_3 (CPU_CORE_MPIDR_EL1_BASE + 0x3)

int32_t wake_slave_core(void);
void slave_core_board_init(void); /* for core board init */
uint64_t hb_kill_slave_core(void);
bool env_is_ready(volatile struct global_data *p_gd);
void show_gd_info(const char *s, const volatile struct global_data *p_gd);
#endif // INCLUDE_CONFIGS_XJ3_CPUS_H_
