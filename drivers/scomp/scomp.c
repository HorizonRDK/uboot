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

int scomp_read_sw_efuse_bnk(enum EFS_TPE type, unsigned int bnk_num)
{
	unsigned int efs_reg_off = 0;

	if (type == EFS_NS) {
		efs_reg_off = fw_base_addr + EFUSE_NS_OFF;
	} else if (type == EFS_S) {
		efs_reg_off = fw_base_addr + EFUSE_S_OFF;
	} else {
		printf("[ERR] %s no such type %d!!!", __FUNCTION__, type);
		return SW_EFUSE_FAIL;
	}

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
int api_efuse_read_data(enum EFS_TPE type, unsigned int bnk_num)
{
	return scomp_read_sw_efuse_bnk(type, bnk_num);
}

int api_efuse_dump_data(enum EFS_TPE type)
{
	unsigned int efs_reg_bnk = 0;
	unsigned int efs_reg_size = 0;
	unsigned int lock_byte = 0;

	if (type == EFS_NS) {
		efs_reg_size = NS_EFUSE_NUM;
	} else if (type == EFS_S) {
		efs_reg_size = S_EFUSE_NUM;
	} else {
		printf("%s no such type %d!!!", __FUNCTION__, type);
		return RET_API_EFUSE_FAIL;
	}

	/* Read Bnk 31, lock bnk */
	lock_byte = scomp_read_sw_efuse_bnk(type, HW_EFUSE_LOCK_BNK);

	if (SW_EFUSE_FAIL == lock_byte) {
		printf("%s line:%d, read sw efuse error.\n",
			__FUNCTION__, __LINE__);
		return RET_API_EFUSE_FAIL;
	}

	/* Dump Bnk 0 ~ 30 */
	for(efs_reg_bnk = 0; efs_reg_bnk < (efs_reg_size -1);  efs_reg_bnk++) {
		printf("Bnk[0x%x] = 0x%x (Lock bit = %d)\n", efs_reg_bnk,
			scomp_read_sw_efuse_bnk(type, efs_reg_bnk),
			((lock_byte >> efs_reg_bnk) & 0x1));
	}

	/* Dump Bnk 31. */
	printf("Bnk[31] = 0x%x\n", lock_byte);

	return RET_API_EFUSE_OK;
}

int sflow_handle_efuse_dumping(void)
{
	int nRet = RET_API_EFUSE_OK;

	printf("efuse: dump all banks\n");

	/* dump secure efuse data */
	nRet = api_efuse_dump_data(EFS_S);
	if (nRet != RET_API_EFUSE_OK) {
		printf("[Error] dump secure efuse data fail !!\n");
		return nRet;
	}

	printf("----------------------------------------------------------\n");

	/* dump normal efuse data */
	nRet = api_efuse_dump_data(EFS_NS);
	if (nRet != RET_API_EFUSE_OK) {
		printf("[Error] dump normal efuse data fail !!\n");
		return nRet;
	}

	printf("==========================================================\n");
	return SFLOW_PASS;
}

