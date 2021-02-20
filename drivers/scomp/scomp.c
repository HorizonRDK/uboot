/*
 *    COPYRIGHT NOTICE
 *    Copyright 2020 Horizon Robotics, Inc.
 *    All rights reserved.
 */
#include <common.h>
#include <stdio.h>
#include <hb_spacc.h>
#include <scomp.h>

static unsigned int fw_base_addr = SEC_REG_BASE;

static unsigned int _hw_efuse_store(unsigned int bnk_num,
		unsigned int data)
{
	unsigned int efs_bank_base;
	unsigned int sta;

	efs_bank_base = HW_NORMAL_EFUSE_BASE;

	// enable efuse
	mmio_write_32(efs_bank_base + HW_EFS_CFG, HW_EFS_CFG_EFS_EN);

	//// Check lock bank before write data
	// set bank addr to bnk31
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, HW_EFUSE_LOCK_BNK);

	// trigger read action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_RD);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY)
			break;
	} while (1);

	// check if bank is valid programmed data
	sta = mmio_read_32(efs_bank_base + HW_EFS_READ_DATA);

	if (sta & (1 << bnk_num)) {
		printf("[ERR] bank %d was locked, it can not be writed.\n", bnk_num);
		return HW_EFUSE_WRITE_LOCKED_BNK;
	}

	//// efuse write data
	// Set bank address to what we want
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, bnk_num);

	// program data
	mmio_write_32(efs_bank_base + HW_EFS_PROG_DATA, data);

	// Trigger program action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_WR);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY_PASS)
			break;
	} while (1);

	//// efuse read data for varification
	// Set bank address to what we want
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, bnk_num);

	// Trigger read action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_RD);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY)
			break;
	} while (1);

	// read data
	sta = mmio_read_32(efs_bank_base + HW_EFS_READ_DATA);

	// verify writed data
	if(sta != data) {
		printf("[ERR] bank %d write fail.\n", bnk_num);
		return HW_EFUSE_WRITE_FAIL;
	}

	//// efuse lock bank
	// set program bank addr to bnk31
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, HW_EFUSE_LOCK_BNK);

	// Write lock to lock_bank (31) for specfic ank HW_EFS_PROG_DATA
	mmio_write_32(efs_bank_base + HW_EFS_PROG_DATA, 1 << bnk_num);

	// Trigger program action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_WR);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY_PASS)
			break;
	} while (1);

	return HW_EFUSE_READ_OK;
}

static unsigned int _hw_efuse_load(unsigned int bnk_num,
		unsigned int *ret)
{
	unsigned int efs_bank_base;
	unsigned int sta, val;

	efs_bank_base = HW_NORMAL_EFUSE_BASE;

	// enable efuse
	mmio_write_32(efs_bank_base + HW_EFS_CFG, HW_EFS_CFG_EFS_EN);
#if 0
	// set bank addr to bnk31
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, HW_EFUSE_LOCK_BNK);

	// trigger read action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_RD);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY)
			break;
	} while (1);

	// check if bank is valid programmed data
	val = mmio_read_32(efs_bank_base + HW_EFS_READ_DATA);

	if (bnk_num == HW_EFUSE_LOCK_BNK) {
		*ret = val;
		return HW_EFUSE_READ_OK;
	}

	if (!(val & (1 << bnk_num))) {
		printf("bank %d not locked, it is not a valid data\n", bnk_num);
		*ret = 0;
		return HW_EFUSE_READ_UNLOCK;
	}
#endif
	// Set bank address to what we want
	mmio_write_32(efs_bank_base + HW_EFS_BANK_ADDR, bnk_num);

	// Trigger read action
	mmio_write_32(efs_bank_base + HW_EFS_CFG,
			HW_EFS_CFG_EFS_EN | HW_EFS_CFG_EFS_PROG_RD);

	// polling efuse ready (check efs_rdy == 1)
	do {
		sta = mmio_read_32(efs_bank_base + HW_EFS_STATUS);

		if (sta & HW_EFS_EFS_RDY)
			break;
	} while (1);

	// read data
	val = mmio_read_32(efs_bank_base + HW_EFS_READ_DATA);

	*ret = val;

	return HW_EFUSE_READ_OK;
}

unsigned int sw_efuse_set_register()
{
	unsigned int efs_bank_off = 0;
	unsigned int i, efs_bank_num, val;


	efs_bank_off = fw_base_addr + EFUSE_NS_OFF;
	efs_bank_num = NS_EFUSE_NUM;

	for (i = 0; i < efs_bank_num; i++) {
		if (_hw_efuse_load(i, &val) == HW_EFUSE_READ_FAIL) {
			printf("[ERR] HW Efuse read fail bnk: %d\n", i);
			return SW_EFUSE_FAIL;
		}
		mmio_write_32(efs_bank_off + i * 4, val);
	}

	return SW_EFUSE_OK;
}

int scomp_write_hw_efuse_bnk(unsigned int bnk_num,
		unsigned int val)
{
	return _hw_efuse_store(bnk_num, val);
}


int scomp_write_sw_efuse_bnk(unsigned int bnk_num,
		unsigned int val)
{
	unsigned int efs_reg_off = 0;

	efs_reg_off = fw_base_addr + EFUSE_NS_OFF;

	mmio_write_32(efs_reg_off + (bnk_num << 2), val);

	return SW_EFUSE_OK;
}

int scomp_read_sw_efuse_bnk(unsigned int bnk_num)
{
	unsigned int efs_reg_off = 0;

	efs_reg_off = fw_base_addr + EFUSE_NS_OFF;

	return mmio_read_32(efs_reg_off + (bnk_num << 2));
}


/*
 *  -------------------------------------------------------
 *  1. spl image header offset (4byte, and padding to 16byte)
 *  -------------------------------------------------------
 *  2. spl image
 *  -------------------------------------------------------
 *  3. key bank image
 *  -------------------------------------------------------
 *  4. spl header: signature_h
 *  -------------------------------------------------------
 *  5. spl header:
 *		signature_h
 *		signature_i
 *		signature_k
 *		image_loadder_address, size
 *		key_bank_address, size
 *  -------------------------------------------------------
 */
int api_efuse_write_data(unsigned int bnk_num,
		unsigned int val)
{
	int nRet = SW_EFUSE_OK;

	/// 1st step write hw efuse
	nRet = scomp_write_hw_efuse_bnk(bnk_num, val);
	if(HW_EFUSE_READ_OK != nRet) {
		printf("[ERR] %s write hw efuse error:%d.\n", __FUNCTION__, nRet);
		return RET_API_EFUSE_FAIL;
	}

	/// 2nd write sw efuse data when 1st step sccess.
	nRet = scomp_write_sw_efuse_bnk(bnk_num, val);
	if(SW_EFUSE_OK != nRet) {
		printf("[ERR] %s write sw efuse error:%d.\n", __FUNCTION__, nRet);
		return RET_API_EFUSE_FAIL;
	}

	return RET_API_EFUSE_OK;
}

int api_efuse_read_data(unsigned int bnk_num)
{
	return scomp_read_sw_efuse_bnk(bnk_num);
}

int api_efuse_dump_data()
{
	unsigned int efs_reg_bnk = 0;
	unsigned int efs_reg_size = 0;
	unsigned int lock_byte = 0;

	efs_reg_size = NS_EFUSE_NUM;

	/* Read Bnk 31, lock bnk */
	lock_byte = scomp_read_sw_efuse_bnk(HW_EFUSE_LOCK_BNK);

	if (SW_EFUSE_FAIL == lock_byte) {
		printf("%s line:%d, read sw efuse error.\n",
				__FUNCTION__, __LINE__);
		return RET_API_EFUSE_FAIL;
	}

	/* Dump Bnk 0 ~ 30 */
	for(efs_reg_bnk = 0; efs_reg_bnk < (efs_reg_size -1);  efs_reg_bnk++) {
		printf("Bnk[0x%x] = 0x%x \n", efs_reg_bnk,
				scomp_read_sw_efuse_bnk(efs_reg_bnk));
	}

	/* Dump Bnk 31. */
	printf("Bnk[31] = 0x%x\n", lock_byte);

	return RET_API_EFUSE_OK;
}

int sflow_handle_efuse_dumping(void)
{
	int nRet = RET_API_EFUSE_OK;

	printf("efuse: dump all banks\n");

	printf("----------------------------------------------------------\n");

	/* dump normal efuse data */
	nRet = api_efuse_dump_data();
	if (nRet != RET_API_EFUSE_OK) {
		printf("[Error] dump normal efuse data fail !!\n");
		return nRet;
	}

	printf("==========================================================\n");
	return SFLOW_PASS;
}

