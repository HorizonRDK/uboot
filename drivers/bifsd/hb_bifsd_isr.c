/*****************************************************************************/
/*                                                                           */
/*                     HozironRobtics MMC MAC SOFTWARE                       */
/*                                                                           */
/*                  Horizon Robtics SYSTEMS LTD                              */
/*                           COPYRIGHT(C) 2018                               */
/*                                                                           */
/*  This program  is  proprietary to  Ittiam  Systems  Private  Limited  and */
/*  is protected under china Copyright Law as an unpublished work. Its use   */
/*  and  disclosure  is  limited by  the terms  and  conditions of a license */
/*  agreement. It may not be copied or otherwise  reproduced or disclosed to */
/*  persons outside the licensee's organization except in accordance with the*/
/*  terms  and  conditions   of  such  an  agreement.  All  copies  and      */
/*  reproductions shall be the property of HorizonRobtics Systems Private    */
/*    Limited and must bear this notice in its entirety.                     */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*  File Name         : hb_bifsd_isr.c                                      */
/*                                                                           */
/*  Description       : Ths file contains the ISR and DSR for MMC            */
/*                      interface.                                           */
/*                                                                           */
/*  List of Functions : hb_bifsd_isr                                        */
/*                      hb_bifsd_dsr                                        */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)                      Changes               */
/*         03 04 2018   HorizonRbotics  shaochuanzhang Draft                 */
/*                                                                           */
/*****************************************************************************/

#ifndef _HB_BIFSD_ISR_H_
#define _HB_BIFSD_ISR_H_
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

#include <asm/arch/hb_bifsd.h>
#include <common.h>
/*****************************************************************************/
/* Global Variable Definitions                                               */
/*****************************************************************************/
#define bif_printf(fmt, args...) printf(fmt, ##args)
/*****************************************************************************/
/* Extern Global Variables                                                   */
/*****************************************************************************/

void mmc_dump(const u8 *buf, u16 len)
{
    int i;

    for (i=0;i<len;i++) {
        if(i%8 == 0)
          bif_printf("  ");
        bif_printf("%02x ", buf[i]);
        if((i+1)%16 == 0)
            bif_printf("\n");
    }
    bif_printf("\n");
}

void mmc_rx_complete_fn(void)
{
    bif_printf("rx comp\n");
}
void mmc_tx_complete_fn(void)
{
    bif_printf("tx comp\n");
}
void mmc_int1_isr(u32 int_status)
{
    u32 val;
    mmc_struct_t *mmc_info = get_mmc_info();

    if(int_status & MMC_IDLE_CMD)
    {
        /* CMD0 received then reinit MMC Card */
        hb_bifsd_init(NULL);
        bifsd_writel(MMC_IDLE_CMD, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_SET_BLOCK_LEN)
    {
        mmc_info->block_len = bifsd_readl(rMMC_BLOCK_LEN);
        bifsd_writel(MMC_SET_BLOCK_LEN, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_SET_BLOCK_CNT)
    {
        val = bifsd_readl(rMMC_ERASE_BLOCK_CNT);
        mmc_info->block_cnt = val & 0x0000ffff;
        bifsd_writel(MMC_SET_BLOCK_CNT, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_INACTIVE_CMD)
    {
        bifsd_writel(MMC_INACTIVE_CMD, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_BLK_READ)
    {
        if(int_status & MMC_MULTI_BLOCK_READ_WRITE)
        {
            if(int_status & MMC_STOP_CMD)
            {
                /* TBD */
            }
            else
            {
            /* multi block read */

            /* argument reg store the address that will be read from master
               now we don't care it */
            val = bifsd_readl(rMMC_ARGUMENT_REG);
            mmc_info->rw_buf = val;
            /* need set dma address for master read */
            bifsd_writel(val, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);

            /* the block count must be send from master */
            if(!mmc_info->block_cnt)
                mmc_info->block_cnt = 8;

            bifsd_writel(mmc_info->block_cnt, rMMC_BLOCK_CNT);
            bifsd_writel(MMC_BLK_READ | MMC_MULTI_BLOCK_READ_WRITE, rMMC_INT_STATUS_1);
            }
        }
        else
        {
            /* Single block read */
            /* read address from host */
            val = bifsd_readl(rMMC_ARGUMENT_REG);
            if(val == 0)
            {
                printf("read addr is 0\n");
                val = 0x7f400000;
            }
            else
            {
                printf("read val=%x\n",val);
            }
            mmc_info->rw_buf = val;
            /* need set dma address for master read */
            bifsd_writel(val, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);
            bifsd_writel(0x01, rMMC_BLOCK_CNT);
            bifsd_writel(MMC_BLK_READ, rMMC_INT_STATUS_1);
        }
        /* wait master dma read completed */
    }
    else if(int_status & MMC_READ_BLOCK_CNT)
    {
        /* read block count reached interrupt*/
        mmc_tx_complete_fn();
        bifsd_writel(MMC_READ_BLOCK_CNT, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_STOP_CMD)
    {
        u32 reg_val;

        /* master stop read */
        bifsd_writel(0, rMMC_DMA_ADDR);

        reg_val = bifsd_readl(rMMC_PROGRAM_REG);
        if(reg_val & MMC_REG_BIT03)
            bifsd_writel(0x21, rMMC_MEM_MGMT);

        bifsd_writel(MMC_STOP_CMD, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_BLK_WRITE)
    {
        if(int_status & MMC_MULTI_BLOCK_READ_WRITE)
        {
            /* WRITE address from master */
            val = bifsd_readl(rMMC_ARGUMENT_REG);
            mmc_info->rw_buf = val;
            /* need set dma address for master write*/
            bifsd_writel(val, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);
            /* the block count must be send from master */
            if(!mmc_info->block_cnt)
                mmc_info->block_cnt = 8;

            bifsd_writel(mmc_info->block_cnt, rMMC_BLOCK_CNT);
            bifsd_writel(MMC_BLK_WRITE | MMC_MULTI_BLOCK_READ_WRITE, rMMC_INT_STATUS_1);

            /* wait master dma write completed */
        }
        else
        {
            /* WRITE address from host */
            val = bifsd_readl(rMMC_ARGUMENT_REG);
            if(val == 0)
            {
                printf("write addr is 0\n");
            }
            else
            {
                printf("write val=%x\n",val);
            }

            mmc_info->rw_buf = val;
            /* need set dma address for master write*/
            /*bifsd_writel(mmc_info->write_buf, rMMC_DMA_ADDR);*/
            bifsd_writel(val, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);
            bifsd_writel(0x01, rMMC_BLOCK_CNT);
            bifsd_writel(MMC_BLK_WRITE, rMMC_INT_STATUS_1);

            /* wait master dma write completed */
        }
    }
    else if(int_status & MMC_WRITE_BLOCK_CNT)
    {
        u32 reg_val;

        reg_val = bifsd_readl(rMMC_PROGRAM_REG);
        if(reg_val & MMC_REG_BIT03)
            bifsd_writel(0x21, rMMC_MEM_MGMT);

        mmc_rx_complete_fn();

        /* master WRITE completed*/
        bifsd_writel(MMC_WRITE_BLOCK_CNT, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_CID_UPDATE)
    {
        update_cid_val();

        bifsd_writel(MMC_CID_UPDATE, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_CSD_UPDATE)
    {
        update_csd_val();

        bifsd_writel(MMC_CSD_UPDATE, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_NUM_WELL_WRITE_BLOCK)
    {
        /* SD mode only,ACMD22 is received */
        bifsd_writel(mmc_info->rw_buf, rMMC_DMA_ADDR);
        bifsd_writel(0x01, rMMC_MEM_MGMT);
        bifsd_writel(MMC_NUM_WELL_WRITE_BLOCK, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_GENERAL_READ_WRITE)
    {
        /*CMD56 is received, argument is the direction of data not address that to read/write */
        val = bifsd_readl(rMMC_ARGUMENT_REG);
        if(val)
        {
            bifsd_writel(mmc_info->rw_buf, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);
        }
        else
        {
            bifsd_writel(mmc_info->rw_buf, rMMC_DMA_ADDR);
            bifsd_writel(0x01, rMMC_MEM_MGMT);
        }

        bifsd_writel(MMC_GENERAL_READ_WRITE, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_BLOCK_COUNT_CLEAR)
    {
        bifsd_writel(MMC_BLOCK_COUNT_CLEAR, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_VOLTAGE_SWITCH)
    {
        /* voltage switch*/
        bifsd_writel(MMC_VOLTAGE_SWITCH, rMMC_INT_STATUS_1);
    }
    else if(int_status &  MMC_GO_PRE_IDLE)
    {
        /* go pre-idel state */
        hb_bifsd_init(NULL);
        bifsd_writel(MMC_GO_PRE_IDLE, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_CMD_61)
    {
        val = bifsd_readl(rMMC_ARGUMENT_REG);
        /* need set dma address for master read */
        bifsd_writel(val, rMMC_DMA_ADDR);
        bifsd_writel(0x01, rMMC_MEM_MGMT);

        /* the block count must be send from master */
        bifsd_writel(mmc_info->read_blk_num, rMMC_BLOCK_CNT);
        bifsd_writel(MMC_CMD_61, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_CMD6_ALWAYS)
    {
        mmc_info->cmd_argu = bifsd_readl(rMMC_ARGUMENT_REG);

        bifsd_writel(0x21, rMMC_MEM_MGMT);
        bifsd_writel(MMC_CMD6_ALWAYS, rMMC_INT_STATUS_1);
    }
    else if(int_status & MMC_CMD55)
    {
        mmc_info->cmd_argu = bifsd_readl(rMMC_ARGUMENT_REG);
        bifsd_writel(MMC_CMD55, rMMC_INT_STATUS_1);
    }
    else
    {
        bif_printf("invalid ISR(%08x)\n",int_status);
        bifsd_writel(int_status, rMMC_INT_STATUS_1);
    }
}
void mmc_int2_isr(u32 int_status)
{
    if(int_status & MMC_CMD12)
    {
        /* Clear Int state */
        bifsd_writel(MMC_CMD12, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_SLEEP_CMD)
    {
        /* Enter sleep state */
        bifsd_writel(0x21, rMMC_MEM_MGMT);
        bifsd_writel(MMC_SLEEP_CMD, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_AWAKE_CMD)
    {
        /* Enter Standby state */
        bifsd_writel(0x21, rMMC_MEM_MGMT);
        bifsd_writel(MMC_AWAKE_CMD, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_UPDATE_EXT_CSD)
    {
        u32 old_val;
        u32 val;
        unsigned char val_offset;
        u32 new_val;
        u32 update_val;
        u32 update_addr = 0;

        val = bifsd_readl(rMMC_ARGUMENT_REG);
        val &= 0x00FF0000;
        val = val >> 16;

        val_offset = val % 4;
        if(val_offset)
        {
            val -= val_offset;
        }
        if(val >= 128)
        {
            update_addr = val - 48;
        }
        else if(val < 80)
        {
            update_addr = val;
        }
        else
        {
            bif_printf("update ext csd register address error\n");
        }
        update_val = bifsd_readl(rMMC_UPDATE_EXT_CSD);
        old_val = bifsd_readl(rMMC_EXTENDED_CSD + update_addr);

        old_val = old_val & (~(0xFF << val_offset*8));

        new_val = update_val << val_offset*8;

        new_val = old_val | new_val;

        bifsd_writel(new_val, rMMC_EXTENDED_CSD + update_addr);

        bifsd_writel(0x21, rMMC_MEM_MGMT);
        bifsd_writel(MMC_UPDATE_EXT_CSD, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_HARDWARE_RESET)
    {
        bifsd_writel(MMC_HARDWARE_RESET, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_CARD_SELECT)
    {
        /* The core moves from stand by state to transfer state */
        bifsd_writel(MMC_CARD_SELECT, rMMC_INT_STATUS_2);
    }
    else if(int_status & MMC_CARD_STATUS_CHANGE)
    {
        /* The core moves from stand by state to transfer state */
        bifsd_writel(MMC_CARD_STATUS_CHANGE, rMMC_INT_STATUS_2);
    }
    else
    {
        bif_printf("invalid int(%08x)\n",int_status);
        bifsd_writel(int_status, rMMC_INT_STATUS_2);
    }
}

void hb_bifsd_isr(void)
{
    u32 int_status_1;
    u32 int_status_2;
    u32 val;

    mmc_struct_t *mmc_info = get_mmc_info();

    /* Get the status of interrupt from INT_STATUS_REG_1 */
    int_status_1 = get_mmc_int1_status();
    /* Get the status of interrupt from INT_STATUS_REG_2 */
    int_status_2 = get_mmc_int2_status();

    /* Protocol read without count */
    if((int_status_1 &  MMC_MULTI_BLOCK_READ_WRITE) && (int_status_2 & MMC_SECURITY_PROTOCOL_READ))
    {
        /* READ address from master */
        val = bifsd_readl(rMMC_ARGUMENT_REG);

        /* need set dma address for master read */
        bifsd_writel(val, rMMC_DMA_ADDR);
        bifsd_writel(0x01, rMMC_MEM_MGMT);

        /* the block count is the slave set */
        bifsd_writel(mmc_info->block_cnt, rMMC_BLOCK_CNT);
        bifsd_writel(MMC_SECURITY_PROTOCOL_READ, rMMC_INT_STATUS_2);
        bifsd_writel(MMC_MULTI_BLOCK_READ_WRITE, rMMC_INT_STATUS_1);
    }
    /* Protocol write without count */
    if((int_status_1 &  MMC_MULTI_BLOCK_READ_WRITE) && (int_status_2 & MMC_SECURITY_PROTOCOL_WRITE))
    {
        /* WRITE address from master */
        val = bifsd_readl(rMMC_ARGUMENT_REG);
        /* need set dma address for master write*/
        bifsd_writel(val, rMMC_DMA_ADDR);
        bifsd_writel(0x01, rMMC_MEM_MGMT);

        /* the block count is the slave set */
        bifsd_writel(mmc_info->block_cnt, rMMC_BLOCK_CNT);
        bifsd_writel(MMC_MULTI_BLOCK_READ_WRITE, rMMC_INT_STATUS_1);
        bifsd_writel(MMC_SECURITY_PROTOCOL_WRITE, rMMC_INT_STATUS_2);
    }

    if(int_status_1 != 0)
    {
        mmc_int1_isr(int_status_1);
    }

    if(int_status_2 != 0)
    {
        mmc_int2_isr(int_status_2);
    }
}

#endif /* HB_BIFSD_ISR_H_*/
