/***************************************************************************
 *			COPYRIGHT NOTICE
 *		Copyright 2020 Horizon Robotics, Inc.
 *			All rights reserved.
 ***************************************************************************/
#include <common.h>
#include <command.h>
#include <console.h>
#include <scomp.h>
#include <linux/errno.h>

static int strtou32(const char *str, unsigned int base, u32 *result)
{
	char *ep;

	*result = simple_strtoul(str, &ep, base);
	if (ep == str || *ep != '\0')
		return -EINVAL;

	return 0;
}

static int confirm_prog(void)
{
	puts("Warning: Programming fuses is an irreversible operation!\n"
			"         This may brick your system.\n"
			"         Use this command only if you are sure of "
			"what you are doing!\n"
			"\nReally perform this fuse programming? <y/N>\n");

	if (confirm_yesno())
		return 1;

	puts("Fuse programming aborted\n");
	return 0;
}

#if 0
static int do_fuse(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *op = argc >= 2 ? argv[1] : NULL;
	u32 bank, cnt, val, type;
	int ret, i;

	if (!strcmp(op, "read")) {
		argc -= 2;
		argv += 2;

		if (argc < 2 || strtou32(argv[0], 0, &type) ||
				strtou32(argv[1], 0, &bank))
			return CMD_RET_USAGE;

		if (argc == 2)
			cnt = 1;
		else if (argc != 3 || strtou32(argv[2], 0, &cnt))
			return CMD_RET_USAGE;

		if (type == 0) {
			printf("Normal efuse read:\n");
		} else if (type == 1) {
			printf("Secure efuse read:\n");
		} else {
			printf("Wrong type enter, please enter 1 or 0\n");
			return CMD_RET_USAGE;
		}

		for (i = 0; i < cnt; i++, bank++) {
			if (!(i % 4))
				printf("\nbank[%d]:", bank);

			val = api_efuse_read_data(type, bank);
			printf(" %.8x", val);
		}
		putc('\n');
	} else if (!strcmp(op, "dump")) {
		argc -= 2;
		argv += 2;

		if (argc != 1 || strtou32(argv[0], 0, &type))
			return CMD_RET_USAGE;

		if (type == 0) {
			printf("Normal efuse dump:\n");
		} else if (type == 1) {
			printf("Secure efuse dump:\n");
		} else {
			printf("Wrong type enter, please enter 1 or 0\n");
			return CMD_RET_USAGE;
		}

		ret = api_efuse_dump_data(type);
		if (ret)
			goto err;
	} else if (!strcmp(op, "prog")) {
		argc -= 2;
		argv += 2;

		if (argc < 2 || strtou32(argv[0], 0, &type) ||
				strtou32(argv[1], 0, &bank) ||
				strtou32(argv[2], 0, &val))
			return CMD_RET_USAGE;

		if (!confirm_prog())
			return CMD_RET_FAILURE;

		ret = api_efuse_write_data(type, bank, val);
		if (ret)
			goto err;
	} else {
		return CMD_RET_USAGE;
	}

	return 0;

err:
	puts("ERROR\n");
	return CMD_RET_FAILURE;
}
#endif

static int do_fuse(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *op = argc >= 2 ? argv[1] : NULL;
	u32 bank, cnt, val;
	int ret, i;

	if (op == NULL) {
		printf("please add efuse parameter\n");
		goto err;
	}
	if (!strcmp(op, "read")) {
		argc -= 2;
		argv += 2;

		if (argc < 1 || strtou32(argv[0], 0, &bank))
			return CMD_RET_USAGE;

		if (argc == 1)
			cnt = 1;
		else if (argc != 2 || strtou32(argv[1], 0, &cnt))
			return CMD_RET_USAGE;

		printf("Normal efuse read:\n");

		for (i = 0; i < cnt; i++, bank++) {
			if (!(i % 4))
				printf("\nbank[%d]:", bank);

			val = api_efuse_read_data(bank);
			printf(" %.8x", val);
		}
		putc('\n');
	} else if (!strcmp(op, "dump")) {
		if (argc != 2)
			return CMD_RET_USAGE;

		printf("Normal efuse dump:\n");

		ret = api_efuse_dump_data();
		if (ret)
			goto err;
	} else if (!strcmp(op, "prog")) {
		argc -= 2;
		argv += 2;

		if (argc < 1 || strtou32(argv[0], 0, &bank) ||
				strtou32(argv[1], 0, &val))
			return CMD_RET_USAGE;

		if (!confirm_prog())
			return CMD_RET_FAILURE;

		ret = api_efuse_write_data(bank, val);
		if (ret)
			goto err;
	} else {
		return CMD_RET_USAGE;
	}

	return 0;

err:
	puts("ERROR\n");
	return CMD_RET_FAILURE;
}
U_BOOT_CMD(
		efuse, CONFIG_SYS_MAXARGS, 0, do_fuse,
		"efuse sub-system",
		"efuse read <bank_num> [<cnt>] - read 1 or 'cnt' fuse banks,\n"
		"    starting at 'bank'\n"
		"efuse dump - dump all banks\n"
		"    starting at 'bank'\n"
		"efuse prog <bank_num> <val> - program a bank with val\n"
		"    starting at 'bank' (PERMANENT)\n"
/*		"efuse read <type> <bank_num> [<cnt>] - read 1 or 'cnt' fuse banks,\n"
		"    starting at 'bank'\n"
		"efuse dump <type> - dump all banks\n"
		"    starting at 'bank'\n"
		"efuse prog <type> <bank_num> <val> - program a bank with val\n"
		"    starting at 'bank' (PERMANENT)\n"
		"type: 0->normal\n"
		"type: 1->secure\n"*/
		);
