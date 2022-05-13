/*
 *    COPYRIGHT NOTICE
 *   Copyright 2020 Horizon Robotics, Inc.
 *    All rights reserved.
*/
#include <common.h>
#include <asm/io.h>
#include <asm/arch/hb_pmu.h>

#include <hb_info.h>

static const char *swinfo_boot_desp[] = {
	"normal", "splonce", "ubootonce",
	"splwait", "ubootwait", "udumptf", "udumpemmc",
	"udumpusb", "udumpfastboot", "unknown"
};
static int do_swinfo(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	u32 s_magic, s_val[4], i;
	void *s_addr;

	if (argc > 1) {
		if (strcmp(argv[1], "reg") == 0)
			s_magic = 0;
		else if (strcmp(argv[1], "mem") == 0)
			s_magic = HB_SWINFO_MEM_MAGIC;
		else
			s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	} else {
		s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	}
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addr = (void *)(HB_SWINFO_MEM_ADDR);
		printf("swinfo: mem -- %p\n", s_addr);
	} else {
		s_addr = (void *)(HB_PMU_SW_REG_00);
		printf("swinfo: reg -- %p\n", s_addr);
	}

	for(i = 0; i < 32; i+=4) {
		s_val[0] = readl(s_addr + (i << 2));
		s_val[1] = readl(s_addr + ((i + 1) << 2));
		s_val[2] = readl(s_addr + ((i + 2) << 2));
		s_val[3] = readl(s_addr + ((i + 3) << 2));
		printf("%02X: %08X %08X %08X %08X\n", i << 2,
				s_val[0], s_val[1], s_val[2], s_val[3]);
	}
#ifdef HB_SWINFO_BOOT_OFFSET
	s_val[0] = readl(s_addr + HB_SWINFO_BOOT_OFFSET) & 0xF;
	i = ARRAY_SIZE(swinfo_boot_desp);
	i = (s_val[0] < i) ? s_val[0] : (i - 1);
	printf("swinfo boot: %d %s\n", s_val[0], swinfo_boot_desp[i]);
#endif
#ifdef HB_SWINFO_DUMP_OFFSET
	s_val[0] = readl(s_addr + HB_SWINFO_DUMP_OFFSET);
	if (s_val[0]) {
		s_val[3] = s_val[0] & 0xFF;
		s_val[2] = (s_val[0] >> 8) & 0xFF;
		s_val[1] = (s_val[0] >> 16) & 0xFF;
		s_val[0] = (s_val[0] >> 24) & 0xFF;
		printf("swinfo dump: %d.%d.%d.%d\n",
				s_val[0], s_val[1], s_val[2], s_val[3]);
	} else {
		printf("swinfo dump: none\n");
	}
#endif

	return CMD_RET_SUCCESS;
}

static int do_swinfo_sel(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	u32 s_magic;

	if (argc > 1) {
		if (strcmp(argv[1], "reg") == 0)
			writel(0x0, HB_SWINFO_MEM_ADDR);
		else if (strcmp(argv[1], "mem") == 0)
			writel(HB_SWINFO_MEM_MAGIC, HB_SWINFO_MEM_ADDR);
		flush_dcache_all();
	}
	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC)
		printf("swinfo sel: mem\n");
	else
		printf("swinfo sel: reg\n");

	return CMD_RET_SUCCESS;
}

static int do_swinfo_reg(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	u32 s_rw, s_i, s_o, s_valr, s_valw;
	void *s_addr = (void *)(HB_PMU_SW_REG_00);

	switch (argc) {
	case 1:
		argc++;
		argv--;
		return do_swinfo(cmdtp, flag, argc, argv);
	case 2:
		s_rw = 0;
		s_i = simple_strtoul(argv[1], NULL, 10);
		break;
	case 3:
		s_rw = 1;
		s_i = simple_strtoul(argv[1], NULL, 10);
		s_valw = simple_strtoul(argv[2], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}

	if (s_i >= 32) {
		printf("swinfo reg: index %d error\n", s_i);
		return CMD_RET_FAILURE;
	}
	s_o = s_i << 2;
	s_addr += s_o;
	if (s_rw) {
		s_rw = readl(s_addr);
		writel(s_valw, s_addr);
		s_valr = readl(s_addr);
		if (s_valr == s_valw) {
			printf("swinfo reg: %d[%02X]: %08X -> %08X\n",
					s_i, s_o, s_rw, s_valr);
		} else {
			printf("swinfo reg: %d[%02X]: %08X -> %08X fail\n",
					s_i, s_o, s_rw, s_valr);
			return CMD_RET_FAILURE;
		}
	} else {
		s_valr = readl(s_addr);
		printf("swinfo reg: %d[%02X]: %08X\n",
				s_i, s_o, s_valr);
	}

	return CMD_RET_SUCCESS;
}

static int do_swinfo_mem(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	u32 s_rw, s_i, s_o, s_valr, s_valw;
	void *s_addr = (void *)(HB_SWINFO_MEM_ADDR);

	switch (argc) {
	case 1:
		argc++;
		argv--;
		return do_swinfo(cmdtp, flag, argc, argv);
	case 2:
		s_rw = 0;
		s_i = simple_strtoul(argv[1], NULL, 10);
		break;
	case 3:
		s_rw = 1;
		s_i = simple_strtoul(argv[1], NULL, 10);
		s_valw = simple_strtoul(argv[2], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}

	if (s_i >= 32) {
		printf("swinfo mem: index %d error\n", s_i);
		return CMD_RET_FAILURE;
	}
	s_o = s_i << 2;
	s_addr += s_o;
	if (s_rw) {
		s_rw = readl(s_addr);
		writel(s_valw, s_addr);
		if (s_i != 0) /* auto magic */
			writel(HB_SWINFO_MEM_MAGIC, HB_SWINFO_MEM_ADDR);
		flush_dcache_all();
		s_valr = readl(s_addr);
		if (s_valr == s_valw) {
			printf("swinfo mem: %d[%02X]: %08X -> %08X\n",
					s_i, s_o, s_rw, s_valr);
		} else {
			printf("swinfo mem: %d[%02X]: %08X -> %08X fail\n",
					s_i, s_o, s_rw, s_valr);
			return CMD_RET_FAILURE;
		}
	} else {
		s_valr = readl(s_addr);
		printf("swinfo mem: %d[%02X]: %08X\n",
				s_i, s_o, s_valr);
	}

	return CMD_RET_SUCCESS;
}

#ifdef HB_SWINFO_BOOT_OFFSET
static int do_swinfo_boot(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	u32 s_magic, s_val, s_rw, s_f;
	void *s_addr;

	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addr = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_BOOT_OFFSET);
		s_f = 1;
	} else {
		s_addr = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_BOOT_OFFSET);
	}

	switch (argc) {
	case 1:
		s_rw = 0;
		break;
	case 2:
		s_rw = 1;
		s_val = simple_strtoul(argv[1], NULL, 10);
		if (s_val >= 16) {
			printf("swinfo boot %d error\n", s_val);
			return CMD_RET_FAILURE;
		}
		break;
	default:
		return CMD_RET_USAGE;
	}

	if (s_rw) {
		s_rw = readl(s_addr) & ~(0xF);
		s_rw |= s_val;
		writel(s_rw, s_addr);
		if (s_f)
			flush_dcache_all();
	}
	s_val = readl(s_addr) & 0xF;
	s_f = ARRAY_SIZE(swinfo_boot_desp);
	s_f = (s_val < s_f) ? s_val : (s_f - 1);
	printf("swinfo boot: %d %s\n", s_val, swinfo_boot_desp[s_f]);

	return CMD_RET_SUCCESS;
}
#endif

#ifdef HB_SWINFO_DUMP_OFFSET
static int do_swinfo_dump(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	u32 s_magic, s_val[4], s_rw, s_f;
	void *s_addr;

	s_magic = readl((void *)HB_SWINFO_MEM_ADDR);
	if (s_magic == HB_SWINFO_MEM_MAGIC) {
		s_addr = (void *)(HB_SWINFO_MEM_ADDR + HB_SWINFO_DUMP_OFFSET);
		s_f = 1;
	} else {
		s_f = 0;
		s_addr = (void *)(HB_PMU_SW_REG_00 + HB_SWINFO_DUMP_OFFSET);
	}

	switch (argc) {
	case 1:
		s_rw = 0;
		break;
	case 2:
		s_rw = 1;
		s_val[0] = simple_strtoul(argv[1], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}

	if (s_rw) {
		writel(s_val[0], s_addr);
		if (s_f)
			flush_dcache_all();
	}
	s_val[0] = readl(s_addr);
	if (s_val[0]) {
		s_val[3] = s_val[0] & 0xFF;
		s_val[2] = (s_val[0] >> 8) & 0xFF;
		s_val[1] = (s_val[0] >> 16) & 0xFF;
		s_val[0] = (s_val[0] >> 24) & 0xFF;
		printf("swinfo dump: %d.%d.%d.%d\n",
				s_val[0], s_val[1], s_val[2], s_val[3]);
	} else {
		printf("swinfo dump: none\n");
	}

	return CMD_RET_SUCCESS;
}
#endif

static cmd_tbl_t cmd_swinfo[] = {
	U_BOOT_CMD_MKENT(info, 2, 0, do_swinfo, "", ""),
	U_BOOT_CMD_MKENT(sel, 2, 1, do_swinfo_sel, "", ""),
	U_BOOT_CMD_MKENT(reg, 3, 1, do_swinfo_reg, "", ""),
	U_BOOT_CMD_MKENT(mem, 3, 1, do_swinfo_mem, "", ""),
#ifdef HB_SWINFO_BOOT_OFFSET
	U_BOOT_CMD_MKENT(boot, 2, 1, do_swinfo_boot, "", ""),
#endif
#ifdef HB_SWINFO_DUMP_OFFSET
	U_BOOT_CMD_MKENT(dump, 2, 1, do_swinfo_dump, "", ""),
#endif
};

static int do_swinfoops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_swinfo, ARRAY_SIZE(cmd_swinfo));

	/* Drop the mmc command */
	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	swinfo, 29, 1, do_swinfoops,
	"swinfo sub system",
	"info [reg/mem] - display info of the swinfo\n"
	"swinfo sel [reg/mem] - select reg/mem\n"
	"swinfo reg [index [value]] - get/set reg\n"
	"swinfo mem [index [value]] - get/set mem\n"
#ifdef HB_SWINFO_BOOT_OFFSET
	"swinfo boot [type] - get/set boot info\n"
#endif
#ifdef HB_SWINFO_DUMP_OFFSET
	"swinfo dump [iphex] - get/set dump ip in hex\n"
#endif
	);
