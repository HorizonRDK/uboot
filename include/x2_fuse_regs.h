/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
*/

#ifndef INCLUDE_X2_FUSE_REGS_H_
#define INCLUDE_X2_FUSE_REGS_H_

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

/* enable read efuse with apb interface */
#define X2_EFUSE_APB_OP_EN       0x1
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

#endif // INCLUDE_X2_FUSE_REGS_H_
