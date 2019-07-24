// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <autoboot.h>
#include <bootretry.h>
#include <cli.h>
#include <console.h>
#include <fdtdec.h>
#include <menu.h>
#include <post.h>
#include <u-boot/sha256.h>
#include <bootcount.h>
#include <asm/arch/ddr.h>
#include <asm/arch/x2_pmu.h>

DECLARE_GLOBAL_DATA_PTR;

#define MAX_DELAY_STOP_STR 32

#ifndef DEBUG_BOOTKEYS
#define DEBUG_BOOTKEYS 0
#endif
#define debug_bootkeys(fmt, args...)		\
	debug_cond(DEBUG_BOOTKEYS, fmt, ##args)

/* Stored value of bootdelay, used by autoboot_command() */
static int stored_bootdelay;
static int stored_dumptype;
extern unsigned int sys_sdram_size;

#if defined(CONFIG_AUTOBOOT_KEYED)
#if defined(CONFIG_AUTOBOOT_STOP_STR_SHA256)

/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;

	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];

	return diff == 0;
}

static int passwd_abort(uint64_t etime)
{
	const char *sha_env_str = env_get("bootstopkeysha256");
	u8 sha_env[SHA256_SUM_LEN];
	u8 sha[SHA256_SUM_LEN];
	char presskey[MAX_DELAY_STOP_STR];
	const char *algo_name = "sha256";
	u_int presskey_len = 0;
	int abort = 0;
	int size = sizeof(sha);
	int ret;

	if (sha_env_str == NULL)
		sha_env_str = CONFIG_AUTOBOOT_STOP_STR_SHA256;

	/*
	 * Generate the binary value from the environment hash value
	 * so that we can compare this value with the computed hash
	 * from the user input
	 */
	ret = hash_parse_string(algo_name, sha_env_str, sha_env);
	if (ret) {
		printf("Hash %s not supported!\n", algo_name);
		return 0;
	}

	/*
	 * We don't know how long the stop-string is, so we need to
	 * generate the sha256 hash upon each input character and
	 * compare the value with the one saved in the environment
	 */
	do {
		if (tstc()) {
			/* Check for input string overflow */
			if (presskey_len >= MAX_DELAY_STOP_STR)
				return 0;

			presskey[presskey_len++] = getc();

			/* Calculate sha256 upon each new char */
			hash_block(algo_name, (const void *)presskey,
				   presskey_len, sha, &size);

			/* And check if sha matches saved value in env */
			if (slow_equals(sha, sha_env, SHA256_SUM_LEN))
				abort = 1;
		}
	} while (!abort && get_ticks() <= etime);

	return abort;
}
#else
static int passwd_abort(uint64_t etime)
{
	int abort = 0;
	struct {
		char *str;
		u_int len;
		int retry;
	}
	delaykey[] = {
		{ .str = env_get("bootdelaykey"),  .retry = 1 },
		{ .str = env_get("bootstopkey"),   .retry = 0 },
	};

	char presskey[MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen(delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

		debug_bootkeys("%s key:<%s>\n",
			       delaykey[i].retry ? "delay" : "stop",
			       delaykey[i].str ? delaykey[i].str : "NULL");
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	do {
		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey[presskey_len++] = getc();
			} else {
				for (i = 0; i < presskey_max - 1; i++)
					presskey[i] = presskey[i + 1];

				presskey[i] = getc();
			}
		}

		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
				memcmp(presskey + presskey_len -
					delaykey[i].len, delaykey[i].str,
					delaykey[i].len) == 0) {
					debug_bootkeys("got %skey\n",
						delaykey[i].retry ? "delay" :
						"stop");

				/* don't retry auto boot */
				if (!delaykey[i].retry)
					bootretry_dont_retry();
				abort = 1;
			}
		}
	} while (!abort && get_ticks() <= etime);

	return abort;
}
#endif

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot 1 - got key string, abort
 */
static int __abortboot(int bootdelay)
{
	int abort;
	uint64_t etime = endtick(bootdelay);

#  ifdef CONFIG_AUTOBOOT_PROMPT
	/*
	 * CONFIG_AUTOBOOT_PROMPT includes the %d for all boards.
	 * To print the bootdelay value upon bootup.
	 */
	printf(CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

	abort = passwd_abort(etime);
	if (!abort)
		debug_bootkeys("key timeout\n");

	return abort;
}

# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */

#ifdef CONFIG_MENUKEY
static int menukey;
#endif

static int __abortboot(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

#ifdef CONFIG_MENUPROMPT
	printf(CONFIG_MENUPROMPT);
#else
	printf("Hit any key to stop autoboot: %2d ", bootdelay);
#endif

	/*
	 * Check if key already pressed
	 */
	if (tstc()) {	/* we got a key press	*/
		(void) getc();  /* consume input	*/
		puts("\b\b\b 0");
		abort = 1;	/* don't auto boot	*/
	}

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			if (tstc()) {	/* we got a key press	*/
				abort  = 1;	/* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
# ifdef CONFIG_MENUKEY
				menukey = getc();
# else
				(void) getc();  /* consume input	*/
# endif
				break;
			}
			udelay(10000);
		} while (!abort && get_timer(ts) < 1000);

		printf("\b\b\b%2d ", bootdelay);
	}

	putc('\n');

	return abort;
}
# endif	/* CONFIG_AUTOBOOT_KEYED */

static int abortboot(int bootdelay)
{
	int abort = 0;

	if (bootdelay >= 0)
		abort = __abortboot(bootdelay);

#ifdef CONFIG_SILENT_CONSOLE
	if (abort)
		gd->flags &= ~GD_FLG_SILENT;
#endif

	return abort;
}

static void process_fdt_options(const void *blob)
{
#if defined(CONFIG_OF_CONTROL) && defined(CONFIG_SYS_TEXT_BASE)
	ulong addr;

	/* Add an env variable to point to a kernel payload, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "kernel-offset", 0);
	if (addr)
		env_set_addr("kernaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));

	/* Add an env variable to point to a root disk, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "rootdisk-offset", 0);
	if (addr)
		env_set_addr("rootaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));
#endif /* CONFIG_OF_CONTROL && CONFIG_SYS_TEXT_BASE */
}

#if defined(X2_SWINFO_BOOT_OFFSET)
static int x2_swinfo_boot_uboot_check(void)
{
	uint32_t s_magic, s_boot, s_f = 0;
	void *s_addr;

	s_magic = readl((void*)X2_SWINFO_MEM_ADDR);
	if (s_magic == X2_SWINFO_MEM_MAGIC) {
		s_addr = (void *)(X2_SWINFO_MEM_ADDR + X2_SWINFO_BOOT_OFFSET);
		s_f = 1;
	} else {
		s_addr = (void *)(X2_PMU_SW_REG_00 + X2_SWINFO_BOOT_OFFSET);
	}
	s_boot = readl(s_addr) & 0xF;
	if (s_boot == X2_SWINFO_BOOT_UBOOTONCE ||
		s_boot == X2_SWINFO_BOOT_UBOOTWAIT) {
		puts("wait for swinfo boot: ");
		if (s_boot == X2_SWINFO_BOOT_UBOOTONCE) {
			puts("ubootonce\n");
			writel(s_boot & (~0xF), s_addr);
			if (s_f)
				flush_dcache_all();
		} else {
			puts("ubootwait\n");
		}
		return 1;
	}

	return 0;
}
#endif

#if defined(X2_SWINFO_DUMP_OFFSET)
static int x2_swinfo_dump_check(void)
{
	uint32_t s_magic, s_boot, s_dump;
	void *s_addrb, *s_addrd;
	uint8_t d_ip[5];
	char dump[128];
	char *s = dump;

	s_magic = readl((void *)X2_SWINFO_MEM_ADDR);
	if (s_magic == X2_SWINFO_MEM_MAGIC) {
		s_addrb = (void *)(X2_SWINFO_MEM_ADDR + X2_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(X2_SWINFO_MEM_ADDR + X2_SWINFO_DUMP_OFFSET);
	} else {
		s_addrb = (void *)(X2_PMU_SW_REG_00 + X2_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(X2_PMU_SW_REG_00 + X2_SWINFO_DUMP_OFFSET);
	}
	s_boot = readl(s_addrb);
	s_dump = readl(s_addrd);
	if (s_boot == X2_SWINFO_BOOT_UDUMPTF) {
		stored_dumptype = 1;
		printf("swinfo dump ddr 0x%x -> tfcard\n", sys_sdram_size);
		s += sprintf(s, "mmc rescan; ");
		s += sprintf(s, "fatwrite mmc 1:0 0x0 /dump_ddr_%x.img 0x%x",
				sys_sdram_size, sys_sdram_size);
		env_set("dumpcmd", dump);
	} else if (s_dump) {
		stored_dumptype = 2;
		d_ip[0] = (s_dump >> 24) & 0xff;
		d_ip[1] = (s_dump >> 16) & 0xff;
		d_ip[2] = (s_dump >> 8) & 0xff;
		d_ip[3] = s_dump & 0xff;
		d_ip[4] = (d_ip[3] == 1) ? 10 : 1;
		printf("swinfo dump ddr 0x%x %d.%d.%d.%d -> %d\n",
			   sys_sdram_size, d_ip[0], d_ip[1], d_ip[2],
			   d_ip[4], d_ip[3]);
		s += sprintf(s, "setenv ipaddr %d.%d.%d.%d;",
					 d_ip[0], d_ip[1], d_ip[2], d_ip[4]);
		s += sprintf(s, "setenv serverip %d.%d.%d.%d;",
					 d_ip[0], d_ip[1], d_ip[2], d_ip[3]);
		s += sprintf(s, "tput 0x0 0x%x dump_ddr_%x.img",
					 sys_sdram_size, sys_sdram_size);
		env_set("dumpcmd", dump);
	} else {
		stored_dumptype = 0;
	}

	return stored_dumptype;
}

static int x2_swinfo_dump_donecheck(int retc)
{
	uint32_t s_magic, s_boot, s_f = 0;
	void *s_addrb, *s_addrd;

	if (retc) {
		printf("swinfo dump ddr error %d.\n", retc);
		return retc;
	}

	s_magic = readl((void *)X2_SWINFO_MEM_ADDR);
	if (s_magic == X2_SWINFO_MEM_MAGIC) {
		s_addrb = (void *)(X2_SWINFO_MEM_ADDR + X2_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(X2_SWINFO_MEM_ADDR + X2_SWINFO_DUMP_OFFSET);
		s_f = 1;
	} else {
		s_addrb = (void *)(X2_PMU_SW_REG_00 + X2_SWINFO_BOOT_OFFSET);
		s_addrd = (void *)(X2_PMU_SW_REG_00 + X2_SWINFO_DUMP_OFFSET);
	}
	if (stored_dumptype == 1) {
		s_boot = readl(s_addrb);
		writel(s_boot & (~0xF), s_addrb);
	} else if (stored_dumptype == 2) {
		writel(0x00, s_addrd);
	} else {
		s_f = 0;
	}
	if (s_f)
		flush_dcache_all();

	printf("swinfo dump ddr done.\n");
	return 0;
}
#endif

const char *bootdelay_process(void)
{
	char *s;
	int bootdelay;

	bootcount_inc();

#if defined(X2_SWINFO_DUMP_OFFSET)
	if(x2_swinfo_dump_check()) {
		s = env_get("dumpcmd");
		stored_bootdelay = 0;
		return s;
	}
#endif

	s = env_get("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

#ifdef CONFIG_OF_CONTROL
	bootdelay = fdtdec_get_config_int(gd->fdt_blob, "bootdelay",
			bootdelay);
#endif

	debug("### main_loop entered: bootdelay=%d\n\n", bootdelay);

#if defined(CONFIG_MENU_SHOW)
	bootdelay = menu_show(bootdelay);
#endif
	bootretry_init_cmd_timeout();

#ifdef CONFIG_POST
	if (gd->flags & GD_FLG_POSTFAIL) {
		s = env_get("failbootcmd");
	} else
#endif /* CONFIG_POST */
	if (bootcount_error())
		s = env_get("altbootcmd");
	else
		s = env_get("bootcmd");

	process_fdt_options(gd->fdt_blob);
	stored_bootdelay = bootdelay;

	return s;
}

void autoboot_command(const char *s)
{
	int retc;
	debug("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

#if defined(X2_SWINFO_BOOT_OFFSET)
	if(x2_swinfo_boot_uboot_check())
		return;
#endif

	if (stored_bootdelay == 0 && s) {
		retc = run_command_list(s, -1, 0);
#if defined(X2_SWINFO_DUMP_OFFSET)
		if(stored_dumptype)
			x2_swinfo_dump_donecheck(retc);
#endif
		return;
	}

	if (stored_bootdelay != -1 && s && !abortboot(stored_bootdelay)) {
#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		int prev = disable_ctrlc(1);	/* disable Control C checking */
#endif

		run_command_list(s, -1, 0);

#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		disable_ctrlc(prev);	/* restore Control C checking */
#endif
	}

#ifdef CONFIG_MENUKEY
	if (menukey == CONFIG_MENUKEY) {
		s = env_get("menucmd");
		if (s)
			run_command_list(s, -1, 0);
	}
#endif /* CONFIG_MENUKEY */
}
