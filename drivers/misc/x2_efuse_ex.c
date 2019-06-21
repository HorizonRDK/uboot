// SPDX-License-Identifier: GPL-2.0+
/*
 * EFUSE driver
 *
 * Copyright (C) 2019, Horizon Robotics, <zenghao.zhang@horizon.ai>
 */
#include <common.h>
#include <fuse_ex.h>
#include <linux/errno.h>
#include <asm/io.h>

#define X2_EFUSE_REG_BASE        0xA1001000
#define X2_SWRST_REG_BASE        0xA1000400

/* define reg offset */
#define X2_EFUSE_APB_DATA_BASE   0x00
#define X2_EFUSE_APB_DATA_LEN    64          /* step=4byte,count=64 */
#define X2_EFUSE_WRAP_DATA_BASE  0x100
#define X2_EFUSE_WRAP_DATA_LEN   32          /* setp=4byte,count=32 */
#define X2_EFUSE_TIME_REG_BASE   0x200
#define X2_EFUSE_TIME_REG_LEN    12
#define X2_EFUSE_EN_REG          0x230
#define X2_EFUSE_ST_REG          0x234
#define X2_EFUSE_RESET_REG       0x238
#define X2_EFUSE_PG_ADDR_REG     0x23C
#define X2_EFUSE_WRAP_DONW_REG   0x240
#define X2_EFUSE_WARP_BISRCEDIS  0x244
#define X2_EFUSE_WRAP_EN_REG     0x248

#define X2_EFUSE_APB_OP_EN       0x1         /* enable read efuse with apb interface */
#define X2_EFUSE_OP_DIS          0x0
#define X2_EFUSE_OP_RD           0x2
#define X2_EFUSE_OP_PG           0x3
#define X2_EFUSE_OP_REPAIR_RST   0x800

#define X2_EFUSE_APB_BANK        0
#define X2_EFUSE_WARP_BANK       1
#define X2_EFUSE_RETRYS          10000
#define X2_EFUSE_WORD_LEN        32         /* bits per word */
#define X2_EFUSE_TOTAL_BIT       2048
#define X2_EFUSE_TIME_333M       0


#define x2efuse_rd(reg)       readl((u8 *)X2_EFUSE_REG_BASE + (reg))
#define x2efuse_wr(reg, val)  writel((val), (u8 *)X2_EFUSE_REG_BASE + (reg))

unsigned int rd_time_cfg[1][12] = {
	{269, 19, 3, 5, 3, 41, 42, 5, 3, 3, 19, 6},
};
unsigned int pg_time_cfg[1][12] = {
	{269, 19, 3, 5, 0, 0, 4002, 5, 0, 3, 18, 6},
};

int fuse_read(u32 bank, u32 word, u32 *val)
{
	unsigned int i = 0, rv;

	switch (bank) {
	case X2_EFUSE_APB_BANK:
		if (word > (X2_EFUSE_APB_DATA_LEN-1)) {
			printf("overflow, total number is %d, word can be 0-%d\n", X2_EFUSE_APB_DATA_LEN, X2_EFUSE_APB_DATA_LEN-1);
			return -EINVAL;
		}

		x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_DIS);
		x2efuse_wr(X2_EFUSE_WRAP_EN_REG, X2_EFUSE_APB_OP_EN);
		for (i=0; i<X2_EFUSE_TIME_REG_LEN; i++)
			x2efuse_wr(X2_EFUSE_TIME_REG_BASE+(i*4), rd_time_cfg[X2_EFUSE_TIME_333M][i]);
		x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_RD);
		*val = x2efuse_rd(X2_EFUSE_APB_DATA_BASE+(word*4));
		x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_DIS);
		break;
	case X2_EFUSE_WARP_BANK:
		if (word > (X2_EFUSE_WRAP_DATA_LEN-1)) {
			printf("overflow, total number is %d, word can be 0-%d\n", X2_EFUSE_WRAP_DATA_LEN, X2_EFUSE_WRAP_DATA_LEN-1);
			return -EINVAL;
		}
		rv = readl((void *)X2_SWRST_REG_BASE);
		rv &= (~X2_EFUSE_OP_REPAIR_RST);
		writel(rv, (void *)X2_SWRST_REG_BASE);
		x2efuse_wr(X2_EFUSE_WRAP_EN_REG, 0x0);
		i = 0;
		do {
			udelay(10);
			rv = x2efuse_rd(X2_EFUSE_WRAP_DONW_REG);
			i++;
		} while((!(rv&0x1)) && (i<X2_EFUSE_RETRYS));
		if (i >= X2_EFUSE_RETRYS) {
			printf("wrap read operate timeout!\n");
			rv = readl((void *)X2_SWRST_REG_BASE);
			rv |= X2_EFUSE_OP_REPAIR_RST;
			writel(rv, (void *)X2_SWRST_REG_BASE);
			return -EIO;
		}
		*val = x2efuse_rd(X2_EFUSE_WRAP_DATA_BASE+(word*4));
		rv = readl((void *)X2_SWRST_REG_BASE);
		rv |= X2_EFUSE_OP_REPAIR_RST;
		writel(rv, (void *)X2_SWRST_REG_BASE);
		break;
	default:
		printf("bank=0:read data with apb  interface.\n");
		printf("bank=1:read data with warp interface.\n");
		return -EINVAL;
		break;
	}

	return 0;
}

int fuse_sense(u32 bank, u32 word, u32 *val)
{
	/* not supported */
	printf("%s is not supported!\n", __func__);
	return -ENOSYS;
}

int fuse_prog(u32 bit, u32 val)
{
	unsigned int i;

	/* bit mode, args bit (0-2047), other args are ignore */
	/* Usage: fuse_x prog <bit> <val> */
	if (bit > (X2_EFUSE_TOTAL_BIT-1)) {
		printf("overflow, total number is %d, bit can be 0-%d.\n", X2_EFUSE_TOTAL_BIT, X2_EFUSE_TOTAL_BIT-1);
		return -EINVAL;
	}

	x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_DIS);
	x2efuse_wr(X2_EFUSE_WRAP_EN_REG, X2_EFUSE_APB_OP_EN);
	for (i=0; i<X2_EFUSE_TIME_REG_LEN; i++)
		x2efuse_wr(X2_EFUSE_TIME_REG_BASE+(i*4), pg_time_cfg[X2_EFUSE_TIME_333M][i]);
	x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_PG);
	x2efuse_wr(X2_EFUSE_PG_ADDR_REG, bit);

	x2efuse_wr(X2_EFUSE_EN_REG, X2_EFUSE_OP_DIS);

	return 0;
}

int fuse_override(u32 bank, u32 word, u32 val)
{
	/* not supported */
	printf("%s is not supported!\n", __func__);
	return -ENOSYS;
}
