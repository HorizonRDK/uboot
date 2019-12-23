/*
 * (C) Copyright 2013 - 2015 Xilinx, Inc.
 *
 * Xilinx Zynq SD Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/arch/hb_bifsd.h>

/*****************************************************************************/
/* static Global Variables                                                   */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
mmc_struct_t mmc_info = {0};

mmc_struct_t *get_mmc_info(void)
{
    return &mmc_info;
}
static u32 hb_bifsd_alloc_mem(size_t size)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	return (u32)noncached_alloc(size, HB_BIFSD_ALIGN);
#else
	return (u32)memalign(HB_BIFSD_ALIGN, size);
#endif
}
void update_cid_val(void)
{
    unsigned char i;
    unsigned char update_flag;
    u32 reg_val[4];
    volatile u32 *cid_reg = (volatile u32 *)rMMC_CID;

     for (i = 0; i < 4; i++)
     {
         reg_val[i] = bifsd_readl(rMMC_CID + 4*i);
     }
     update_flag = reg_val[3] & 0xFF000000;
     if(update_flag)
     {
         return;
     }
     else
     {
         for (i = 0; i < 4; i++)
         {
             bifsd_writel(reg_val[i], rMMC_CID + 4*i);
         }
         cid_reg[3] |= 0x01000000;
     }
     bifsd_writel(0x01, rMMC_MEM_MGMT);
     return;
}
void update_csd_val(void)
{
    bifsd_writel(0x21, rMMC_MEM_MGMT);

    return;
}
void mmc_cid_config(CARD_TYPE_T card_type)
{
    int i;
    int tbl_size;
    u32 *cid_tbl_val = NULL;
    const u32 cid_sd_card_tbl_val[] = {
        /* CID Tbl Addr Ofst    VALUE to be initialised */
        /*0x00*/         0xc3aa004a,
        /*0x04*/         0x3610432c,
        /*0x08*/         0x53483235,
        /*0x0C*/         0x00015048,
    };
    const u32 cid_mmc_4_4_card_tbl_val[] = {
        /* CID Tbl Addr Ofst    VALUE to be initialised */
        /*0x00*/         0xc3aa004a,
        /*0x04*/         0x3610432c,
        /*0x08*/         0x53483235,
        /*0x0C*/         0x00010048,
    };
    const u32 cid_mmc_4_2_card_tbl_val[] = {
        /* CSD Tbl Addr Ofst VALUE to be initialised */
        /*0x00*/         0xc3aa004a,
        /*0x04*/         0x3610432c,
        /*0x08*/         0x53483235,
        /*0x0C*/         0x00015048,
    };

    switch(card_type)
    {
    case sd_card:
    case sdhc_card:
    case sdxc_card:
        cid_tbl_val = (u32 *)&cid_sd_card_tbl_val;
        break;
    case mmc_card_4_2:
        cid_tbl_val = (u32 *)&cid_mmc_4_2_card_tbl_val;
        break;
    case mmc_card_4_4:
    case mmc_card_4_5:
    case mmc_card_5_0:
        cid_tbl_val = (u32 *)&cid_mmc_4_4_card_tbl_val;
        break;
    default:
        debug("card type falt\n");
        break;
    }

    tbl_size = 4;
    for (i = 0; i < tbl_size; i++) {
        bifsd_writel(cid_tbl_val[i], rMMC_CID + 4*i);
    }
}
void mmc_csd_config(CARD_TYPE_T card_type)
{
    int i;
    u32 *csd_tbl_val;

    const u32 csd_sd_card_tbl_val [] = {
        /* CSD Tbl Addr Ofst    VALUE to be initialised */
        /*0x00*/         0x08966004,
        /*0x04*/         0xffc00344,
        /*0x08*/         0x325f5980,
        /*0x0C*/         0x000026ff,
    };
    const u32 csd_sdhc_card_tbl_val [] = {
        /* CSD Tbl Addr Ofst    VALUE to be initialised */
        /*0x00*/         0x08966004,
        /*0x04*/         0xffc00344,
        /*0x08*/         0x325f5980,
        /*0x0C*/         0x004026ff,
    };

    const u32 csd_mmc_card_tbl_val [] = {
        /* CSD Tbl Addr Ofst    VALUE to be initialised */
        /*0x00*/         0xe7966004,
        /*0x04*/         0xffc0031c,
        /*0x08*/         0x2a3f599b,
        /*0x0C*/         0x009086ff,
    };

    switch(card_type)
    {
    case sd_card:
        csd_tbl_val = (u32 *)&csd_sd_card_tbl_val;
        break;
    case sdhc_card:
    case sdxc_card:
        csd_tbl_val = (u32 *)&csd_sdhc_card_tbl_val;
        break;
    case mmc_card_4_2:
    case mmc_card_4_4:
    case mmc_card_4_5:
    case mmc_card_5_0:
        csd_tbl_val = (u32 *)&csd_mmc_card_tbl_val;
        break;
    default:
        return;
    }
    for (i = 0; i < 4; i++) {
        bifsd_writel(csd_tbl_val[i], rMMC_CSD + 4*i);
    }
}

void mmc_extended_csd_config(CARD_TYPE_T mmc_type)
{
    int i;
    int tbl_size;
    u32 *csd_tbl_val;
    const u32 ext_csd_mmc_4_2_tbl_val [] = {
        /* Extend CSD Tbl Addr Ofst VALUE to be initialised */
        /*CSD_SD_STANDARD_CARD:192bits  */
        /*0x00*/         0x00000000,
        /*0x04*/         0x02020000,
        /*0x08*/         0x55555502,
        /*0x0C*/         0x461e1e55,
        /*0x10*/         0x008c8c46,
        /*0x14*/         0x00000000,
    };

    const u32 ext_csd_mmc_4_4_tbl_val [] = {
        /* Extend CSD Tbl Addr Ofst VALUE to be initialised */
        /*CSD_SD_STANDARD_CARD:624bits */
        /*0x00*/         0x00000000,
        /*0x04*/         0x00000000,
        /*0x08*/         0x00000000,
        /*0x0C*/         0x00000000,
        /*0x10*/         0x00000000,
        /*0x14*/         0x00000000,
        /*0x18*/         0x00000000,
        /*0x1c*/         0x00000000,
        /*0x20*/         0x00480000,
        /*0x24*/         0x02020300,
        /*0x28*/         0x55555555,
        /*0x2c*/         0x46461e1e,
        /*0x30*/         0x00008c8c,
        /*0x34*/         0x01010000,
        /*0x38*/         0x00010001,
        /*0x3c*/         0x01010100,
        /*0x40*/         0x00000000,
        /*0x44*/         0x00000000,
        /*0x48*/         0x00000000,
    };

    const u32 ext_csd_mmc_4_5_tbl_val [] = {
        /* Extend CSD Tbl Addr Ofst VALUE to be initialised */
        /*CSD_SD_STANDARD_CARD:1168bits*/

        /*0x00*/         0x00000000,
        /*0x04*/         0x00000000,
        /*0x08*/         0x00000000,
        /*0x0c*/         0x00000000,
        /*0x10*/         0x00000000,
        /*0x14*/         0x00000000,
        /*0x18*/         0x00000000,
        /*0x1c*/         0x00000000,
        /*0x20*/         0x00000000,
        /*0x24*/         0x00000000,
        /*0x28*/         0x00000000,
        /*0x2c*/         0x00000000,
        /*0x30*/         0x00000000,
        /*0x34*/         0x00000000,
        /*0x38*/         0x00000000,
        /*0x3c*/         0x00000000,
        /*0x40*/         0x00000000,
        /*0x44*/         0x00000000,
        /*0x48*/         0x00000000,
        /*0x4c*/         0x00000148,
        /*0x50*/         0x03000000,
        /*0x54*/         0x00000202,
        /*0x58*/         0x55555555,
        /*0x5c*/         0x46461e1e,
        /*0x60*/         0x00008c8c,
        /*0x64*/         0x01010000,
        /*0x68*/         0x01010001,
        /*0x6c*/         0x01010100,
        /*0x70*/         0x00000001,
        /*0x74*/         0x00000000,
        /*0x78*/         0x00000000,
        /*0x7c*/         0x00000000,
        /*0x80*/         0x00000001,
        /*0x84*/         0x00000000,
        /*0x88*/         0x00000000,
        /*0x8c*/         0x00000000,
        /*0x90*/         0x00000000,
    };

    const u32 ext_csd_mmc_5_0_tbl_val [] = {
        /* Extend CSD Tbl Addr Ofst VALUE to be initialised */
        /*CSD_SD_STANDARD_CARD:1784bits */
        /*0xA4*/           0x00000000,
        /*0xA8*/           0x00000000,
        /*0xAC*/           0x00000000,
        /*0xB0*/           0x00000000,
        /*0xB4*/           0x00000000,
        /*0xB8*/           0x00000000,
        /*0xBC*/           0x00000000,
        /*0xC0*/           0x00000000,
        /*0xC4*/           0x00000000,
        /*0xC8*/           0x00000000,
        /*0xCC*/           0x00000000,
        /*0xD0*/           0x00000000,
        /*0xD4*/           0x00000000,
        /*0xD8*/           0x00000000,
        /*0xDC*/           0x00000000,
        /*0xE0*/           0x00000000,
        /*0xE4*/           0x00000000,
        /*0xE8*/           0x00000000,
        /*0xEC*/           0x00000000,
        /*0xF0*/           0x00000000,
        /*0xF4*/           0x00000000,
        /*0xF8*/           0x00000000,
        /*0xFC*/           0x48000000,
        /*0x100*/           0x00000001,
        /*0x104*/           0x02030000,
        /*0x108*/           0x55000002,
        /*0x10C*/           0x1e555555,
        /*0x110*/           0x8c46461e,
        /*0x114*/           0x0000008c,
        /*0x118*/           0x01000100,
        /*0x11C*/           0x01000100,
        /*0x120*/           0x01010001,
        /*0x124*/           0x00000101,
        /*0x128*/           0x00000000,
        /*0x12C*/           0x00000000,
        /*0x130*/           0x00000000,
        /*0x134*/           0x00020007,
        /*0x138*/           0x0a0a1f13,
        /*0x13C*/           0x8888eeee,
        /*0x140*/           0x460f1e00,
        /*0x144*/           0x0014780f,
        /*0x148*/           0x03a3e000,
        /*0x14C*/           0x0a0a1410,
        /*0x150*/           0x09010108,
        /*0x154*/           0x00200808,
        /*0x158*/           0x55c8f400,
        /*0x15C*/           0x0a640001,
        /*0x160*/           0x99eeeeee,
        /*0x164*/           0x00001e01,
        /*0x168*/           0x32000000,
        /*0x16C*/           0x0000000a,
        /*0x170*/           0x0002ee00,
        /*0x174*/           0x00000000,
        /*0x178*/           0x00000000,
        /*0x17C*/           0x01202001,
        /*0x180*/           0x00000001,
    };

    switch(mmc_type)
    {
    case mmc_card_4_2:
        csd_tbl_val = (u32 *)&ext_csd_mmc_4_2_tbl_val;
        tbl_size = sizeof(ext_csd_mmc_4_2_tbl_val)/sizeof(ext_csd_mmc_4_2_tbl_val[0]);
        break;
    case mmc_card_4_4:
        csd_tbl_val = (u32 *)&ext_csd_mmc_4_4_tbl_val;
        tbl_size = sizeof(ext_csd_mmc_4_4_tbl_val)/sizeof(ext_csd_mmc_4_4_tbl_val[0]);
        break;
    case mmc_card_4_5:
        csd_tbl_val = (u32 *)&ext_csd_mmc_4_5_tbl_val;
        tbl_size = sizeof(ext_csd_mmc_4_5_tbl_val)/sizeof(ext_csd_mmc_4_5_tbl_val[0]);
        break;
    case mmc_card_5_0:
        csd_tbl_val = (u32 *)&ext_csd_mmc_5_0_tbl_val;
        tbl_size = sizeof(ext_csd_mmc_5_0_tbl_val)/sizeof(ext_csd_mmc_5_0_tbl_val[0]);
        break;
    default:
        return;
    }

    for (i = 0; i < tbl_size; i++) {
        bifsd_writel(csd_tbl_val[i], rMMC_EXTENDED_CSD + 4*i);
    }
}
void mmc_set_sd_status_reg(void)
{
    int i;
    const u32 sd_status_tbl_val [] = {
        /*0x00*/         0x01160000,
        /*0x04*/         0x02199000,
        /*0x08*/         0x00000032,
        /*0x0C*/         0x00000000,
    };
    for (i = 0; i < 4; i++) {
        bifsd_writel(sd_status_tbl_val[i], rMMC_SD_STATUS + 4*i);
    }
}
void mmc_set_sd_scr_reg(CARD_TYPE_T card_type)
{
    int i;
    u32 *scr_tbl_val;
    const u32 sd_tbl_val [] = {
        /*0x00*/         0x00000000,
        /*0x04*/         0x01b50000,
    };
    const u32 sdhc_tbl_val [] = {
        /*0x00*/         0x00000000,
        /*0x04*/         0x02b50000,
    };
    const u32 sdxc_tbl_val [] = {
        /*0x00*/         0x00000000,
        /*0x04*/         0x02b58003,
    };
    switch(card_type)
    {
    case sd_card:
        scr_tbl_val = (u32 *)&sd_tbl_val;
        break;
    case sdhc_card:
        scr_tbl_val = (u32 *)&sdhc_tbl_val;
        break;
    case sdxc_card:
        scr_tbl_val = (u32 *)&sdxc_tbl_val;
        break;
    default:
        return;
    }

    for (i = 0; i < 2; i++) {
        bifsd_writel(scr_tbl_val[i], rMMC_SD_SCR + 4*i);
    }
}

void device_mode_select(CARD_TYPE_T card_type)
{
    u32 reg_val = 0;

    reg_val = bifsd_readl(rMMC_PROGRAM_REG);
    reg_val &=0xFFFFFFFC;

    switch(card_type)
    {
    case sd_card:
    case sdhc_card:
    case sdxc_card:
        reg_val |= MMC_REG_BIT00;
        break;
    case mmc_card_4_2:
    case mmc_card_4_4:
    case mmc_card_4_5:
    case mmc_card_5_0:
        reg_val |= MMC_REG_BIT01;
        break;
    default:
        printf("card type fault\n");
        break;
    }

    reg_val |= MMC_REG_BIT04;

    bifsd_writel(reg_val, rMMC_PROGRAM_REG);
}
void dev_info_init(void)
{
    mmc_struct_t *mmc_info = (mmc_struct_t *)get_mmc_info();
    mmc_info->rw_buf = (u32)hb_bifsd_alloc_mem(256*1024);

    mmc_info->block_len = 512;
    mmc_info->block_cnt = SD_MMC_BLK_CNT;
}

/* Initialize BIFSD */
void hb_bifsd_init(struct udevice *dev)
{
    bifsd_pin_mux_config();
    enable_mmc_int_1();
    enable_mmc_int_2();
    mmc_set_power_up(CARD_MODE);
    mmc_config_ocr_reg(CARD_MODE);
    mmc_extended_csd_config(CARD_MODE);
    mmc_set_hard_reset_cnt();
    mmc_csd_config(CARD_MODE);
    mmc_cid_config(CARD_MODE);
    mmc_set_out_range_addr();
    if((CARD_MODE == sd_card)||(CARD_MODE == sdhc_card)||(CARD_MODE == sdxc_card))
    {
        mmc_set_sd_status_reg();
        mmc_set_sd_scr_reg(CARD_MODE);
        enable_sd_security_interrupt();
        set_security_block_count();
    }

    device_mode_select(CARD_MODE);

#ifndef HUGO_PLM
    bifsd_config_timing();
#endif

#ifdef BIFSD_ACC_ENABLE
    mmc_disable_acc_bypass();
#endif
    card_power_up();
}
void hb_bifsd_initialize(void)
{
    hb_bifsd_init(NULL);
    dev_info_init();
}

#ifdef CONFIG_DM_BIFSD
static int hb_bifsd_probe(struct udevice *dev)
{
	hb_bifsd_init(dev);
	return 0;
}

static const struct udevice_id hb_bifsd_ids[] = {
	{ .compatible = "hobot,bifsd" },
	{ }
};

U_BOOT_DRIVER(hb_bifsd_drv) = {
	.name		= "hb_bifsd_drv",
	.of_match	= hb_bifsd_ids,
	.probe		= hb_bifsd_probe,
    .flags = DM_FLAG_PRE_RELOC,
};
#endif
