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
/*  File Name         : mmc_bifsd.h                                        */
/*                                                                           */
/*  Description       : This file contains all the declarations related to   */
/*                      MMC host interface.                                  */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         04 02 2018   shaochuanzhang@hobot.cc  Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef X2_BIFSD_H
#define X2_BIFSD_H

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include <asm/io.h>
#include <configs/x2_fpga.h>
#include <asm/system.h>
/*****************************************************************************/
/* Constants                                                                 */
/*****************************************************************************/

/* Test for HUGO HOST platform only*/
#define HUGO_PLM

/* GPIO PIN MUX */
#define PIN_MUX_BASE    0xA6003000
#define GPIO1_CFG ((void *)PIN_MUX_BASE + 0x10)
#define GPIO1_DIR ((void *)PIN_MUX_BASE + 0x18)
#define GPIO1_VAL ((void *)PIN_MUX_BASE + 0x1C)

#define ACC_BYPASS_BASE    0xA1000000
/* MMC AHB related Registers */
#define MMC_AHB_BASE    0xA1007000

#define rMMC_PROGRAM_REG          ((void *)MMC_AHB_BASE + 0x00)
#define rMMC_OCR                  ((void *)MMC_AHB_BASE + 0x04)
#define rMMC_CSD                  ((void *)MMC_AHB_BASE + 0x08)
#define rMMC_CID                  ((void *)MMC_AHB_BASE + 0x18)
#define rMMC_CARD_STATE           ((void *)MMC_AHB_BASE + 0x28)
#define rMMC_SD_SCR               ((void *)MMC_AHB_BASE + 0x2C)
#define rMMC_OUT_RANGE_ADDR       ((void *)MMC_AHB_BASE + 0x34)
#define rMMC_INT_ENABLE_1         ((void *)MMC_AHB_BASE + 0x38)
#define rMMC_INT_STATUS_1         ((void *)MMC_AHB_BASE + 0x3C)
#define rMMC_MEM_MGMT             ((void *)MMC_AHB_BASE + 0x40)
#define rMMC_ARGUMENT_REG         ((void *)MMC_AHB_BASE + 0x44)
#define rMMC_DMA_ADDR             ((void *)MMC_AHB_BASE + 0x48)
#define rMMC_BLOCK_CNT            ((void *)MMC_AHB_BASE + 0x4C)
#define rMMC_PASSWORD             ((void *)MMC_AHB_BASE + 0x50)
#define rMMC_SD_STATUS            ((void *)MMC_AHB_BASE + 0x60)
#define rMMC_BLOCK_LEN            ((void *)MMC_AHB_BASE + 0x70)
#define rMMC_ERASE_BLOCK_CNT      ((void *)MMC_AHB_BASE + 0x74)
#define rMMC_ERASE_START_ADDR     ((void *)MMC_AHB_BASE + 0x78)
#define rMMC_ERASE_END_ADDR       ((void *)MMC_AHB_BASE + 0x7C)
#define rMMC_SET_WRITE_PROTECT    ((void *)MMC_AHB_BASE + 0x80)
#define rMMC_CLEAR_WRITE_PROTECT  ((void *)MMC_AHB_BASE + 0x84)
#define rMMC_HARD_RESET_CNT       ((void *)MMC_AHB_BASE + 0x8C)
#define rMMC_DMA_CNT              ((void *)MMC_AHB_BASE + 0x90)
#define rMMC_UPDATE_EXT_CSD       ((void *)MMC_AHB_BASE + 0x94)
#define rMMC_BOOT_BLOCK_CNT       ((void *)MMC_AHB_BASE + 0x98)
#define rMMC_INT_ENABLE_2         ((void *)MMC_AHB_BASE + 0x9C)
#define rMMC_INT_STATUS_2         ((void *)MMC_AHB_BASE + 0xA0)
#define rMMC_EXTENDED_CSD         ((void *)MMC_AHB_BASE + 0xA4)
#define rMMC_PASSWORD_LEN         ((void *)MMC_AHB_BASE + 0x240)
#define rMMC_POWER_UP             ((void *)MMC_AHB_BASE + 0x244)
#define rMMC_PKT_CNT              ((void *)MMC_AHB_BASE + 0x254)
#define rMMC_TIMING               ((void *)MMC_AHB_BASE + 0x258)
#define rMMC_QUEUE_STATUS         ((void *)MMC_AHB_BASE + 0x268)
#define rMMC_FIFO_READ            ((void *)MMC_AHB_BASE + 0x26C)

#define rMMC_SD_SECURITY_INT_ENABLE  ((void *)MMC_AHB_BASE + 0x248)
#define rMMC_SD_SECURITY_INT_STATUS  ((void *)MMC_AHB_BASE + 0x24C)
#define rMMC_BLOCK_COUNT_SECURITY    ((void *)MMC_AHB_BASE + 0x250)
#define rMMC_ACC_CONFIG            ((void *)MMC_AHB_BASE + 0x590)

#define OUT_RANGE_ADDR 0x84000000
#define SD_MMC_BLK_CNT 32
#define SD_SECURITY_INT_ENABLE_VAL 0x00000FFF

/* Interrupt status register_1 */
#define MMC_CSD_UPDATE 	    (1<<0)	/* Host Send CSD command, cmd27 is received */
#define MMC_CID_UPDATE 	    (1<<1)	/* Host Send CID command, cmd26 is received */
#define MMC_SET_BLOCK_CNT   (1<<2)	/* cmd23 is received */
#define MMC_STOP_CMD	    (1<<3)	/* cmd12 is received */
#define MMC_IDLE_CMD        (1<<4)	/* Set device to idle,cmd0 is received */
#define MMC_INACTIVE_CMD    (1<<5)  /* Go inactive,cmd15 is received */
#define MMC_SET_BLOCK_LEN   (1<<6)	/* Set block length cmd16 is received */
#define MMC_CMD6_ALWAYS    	(1<<7)	/* cmd6 is received */
#define MMC_CMD_61      	(1<<8)	/* Vendor cmd61 is received */
#define MMC_CMD_40      	(1<<9)	/* Cmd40 is received */
#define MMC_BLK_READ        (1<<10)	/* Read CMD17/18 is received */
#define MMC_BLK_WRITE       (1<<11)	/* Write CMD24/25 is received */
#define MMC_WRITE_BLOCK_CNT (1<<12)	/* inform the successful completion of a write block transfer */
#define MMC_READ_BLOCK_CNT   (1<<13)	/* Inform successful completion of a read block transfer */
#define MMC_VOLTAGE_SWITCH	   (1<<14)	/* MMC CMD8 or SD CMD11 is received, Switch 3.3V to 1.8V */
#define MMC_SPEED_CLASS_CTL	   (1<<15)	/* Speed class control CMD20 is received */
#define MMC_CMD55	           (1<<16)	/* CMD55 is received */
#define MMC_NUM_WELL_WRITE_BLOCK		(1<<17)
#define MMC_START_ADDR_ERASE   (1<<18)  /* Start address of data to be erased CMD32 or CMD35 received */
#define MMC_END_ADDR_ERASE	   (1<<19)  /* End address to be erased CMD33 or CMD38 received */
#define MMC_ERASE_CMD		   (1<<20)  /* Erase CMD38 is received */
#define MMC_FORCE_ERASE		   (1<<21)  /* Erase the entire card contents bit is set in received CMD42 */
#define MMC_SET_PASSWD		   (1<<22) /* New password reception bit is set in the received CMD42 */
#define MMC_CLEAR_PASSWD	   (1<<23)  /* when clear the password contents bit is set in the received CMD42.*/
#define MMC_LOCK_CARD	       (1<<24)  /* when lock the card bit is set in the received CMD42*/
#define MMC_UPLOCK_CARD 	   (1<<25)  /* unlock the card bit is set in the received CMD42 */
#define MMC_WRITE_PROTECT	   (1<<26)  /* set write protect command [CMD28] is received */
#define MMC_CLEAR_PROTECT	   (1<<27)  /* clear write protect command [CMD29] is received */
#define MMC_WRITE_PROT_STATUS  (1<<28)  /* send_write_prot command to get write protection group status [CMD30] is received */
#define MMC_GENERAL_READ_WRITE	       (1<<29)  /* general read/write command [CMD56] is received */

/* This interrupt bit is set when CMD23 reception is not followed by
 the reception of CMD18 or CMD25. This interrupt informs the firmware to clear the set block count
*/
#define MMC_BLOCK_COUNT_CLEAR	       (1<<30)

/* This interrupt bit is set when Multi block
write/read commands [CMD18 or CMD25] is received.*/
#define MMC_MULTI_BLOCK_READ_WRITE	   (1<<31)

/* Interrupt status register_2 */

/*BIT0 i) if command line is low for 74 clocks then Boot Start interrupt is set
ii) if CMD0 with argument FFFFFFFA is received then Boot Start interrupt is set.*/
#define MMC_BOOT_START		           (1<<0)

/* when the host signals the boot stop operation by making the command line high.*/
#define MMC_BOOT_STOP		           (1<<1)

/* when sleep/awake bit is set in the received command [CMD5] */
#define MMC_SLEEP_CMD		           (1<<2)

/* This interrupt bit is set when sleep/awake bit is cleared by the received command [CMD5]*/
#define MMC_AWAKE_CMD		           (1<<3)

/* the host allows the card to perform background operation*/
#define MMC_BKOPS_START		           (1<<4)

/* This interrupt bit is set to stop the data transfer when the card is in programming state*/
#define MMC_HIGH_PRIORITY		       (1<<5)

/* his bit is set to indicate CRC error is occured in the received data block*/
#define MMC_DATA_CRC_ERR		       (1<<6)

/* This bit is set to indicate CRC error is occured in the received Command*/
#define MMC_CMD_CRC_ERR		           (1<<7)

/* This bit is set when CMD12 is received from Host*/
#define MMC_CMD12		               (1<<8)

/* This bit is set when CMD7 is received with card RCA to move the card to transfer state*/
#define MMC_CARD_SELECT		           (1<<9)

/* This bit is set when CMD7 is received with different RCA other than card RCA to move the card to stand-by state*/
#define MMC_CARD_DESELECT		       (1<<10)

/*This interrupt bit is set when sanitize start bit is set in the received CMD6*/
#define MMC_SANITIZE_START		       (1<<11)

/*This interrupt bit is set when flush cache bit is set in the received CMD6*/
#define MMC_flUSH_CACHE		           (1<<12)

/* his interrupt bit is set when tcase support interrupt bit is set in the received CMD6 */
#define MMC_TCASE_SUPPORT		       (1<<13)

/* This interrupt bit is set when packed command bit is set the received CMD23 */
#define MMC_PACKED_CMD		           (1<<14)

/* This interrupt bit is set when the card received CMD49 command in line */
#define MMC_REAL_TIME_CLK		       (1<<15)

/* This interrupt bit is set when Get write protect type [CMD31] is received] */
#define MMC_GET_WRITE_PROT		       (1<<16)

/* This interrupt bit is set when Partition setting completed bit is set in the received CMD6 */
#define MMC_PARTITION_SETTING_COMPLETED	(1<<17)

/* This interrupt bit is set when set interrupt
for Update EXT_CSD register (MMC) switch command (SD) is set in the CMD6*/
#define MMC_UPDATE_EXT_CSD		       (1<<18)

/* This interrupt bit is set when the reset last
for one micro second duration */
#define MMC_HARDWARE_RESET		       (1<<19)

/* This interrupt bit is set when CMD0 is received with argument[0xF0F0F0F0] */
#define MMC_GO_PRE_IDLE		           (1<<20)

/* This interrupt bit is set when the card completes the packed data transfer */
#define MMC_PACKED_COMPLETION		   (1<<21)

/* This interrupt bit is set when the card detects error in the packet header */
#define MMC_PACKED_FAILURE		       (1<<22)

/* This interrupt bit is set when there is any change in the SD eMMC card state register */
#define MMC_CARD_STATUS_CHANGE		   (1<<23)

/* This interrupt bit is set when CMD53 is received */
#define MMC_SECURITY_PROTOCOL_READ	   (1<<24)

/* This interrupt bit is set when CMD54 is received */
#define MMC_SECURITY_PROTOCOL_WRITE	   (1<<25)

/* The interrupt bit is set when the host con-
troller defines the cmd16 block length less
then lock_unlock card data structure and
when crc error detected in the cmd42 data */
#define MMC_PWD_CMD_FAIL		       (1<<26)

/* This interrupt bit is set when CMD1 is
received in valid state */
#define MMC_CMD1		               (1<<27)

/* cmd48_interrupt */
#define MMC_CMD48		               (1<<28)

/* cmd47_interrupt */
#define MMC_CMD47		               (1<<29)

/* cmd46_interrupt */
#define MMC_CMD46		               (1<<30)

/* This interrupt is generated when cmd
queue fifo is not empty i.e,valid cmd44 &
cmd45 is written in fifo */
#define MMC_FIFO_NOT_EMPTY		       (1<<31)

#define NAC_NON_HS_200_400_VAL (2)
#define NAC_HS_200_400_VAL (5)
#define NCR_VALUE (0)
#define NCRC_VALUE (3)

#define MMC_CPU_CLOCK 40000000
#define HR_TIMER_BASE 0xA1002000
#define BIFSD_IRQ_NUM (49)

#define BIFSD_BLOCK_SIZE (512)
#define X2_BIFSD_ALIGN 32

#define BIFSD_ACC_ENABLE

/***********************************************************************
 * BIT Define
 **********************************************************************/
#define     MMC_REG_BIT00            (1 << 0)
#define     MMC_REG_BIT01            (1 << 1)
#define     MMC_REG_BIT02            (1 << 2)
#define     MMC_REG_BIT03            (1 << 3)
#define     MMC_REG_BIT04            (1 << 4)
#define     MMC_REG_BIT05            (1 << 5)
#define     MMC_REG_BIT06            (1 << 6)
#define     MMC_REG_BIT07            (1 << 7)
#define     MMC_REG_BIT08            (1 << 8)
#define     MMC_REG_BIT09            (1 << 9)
#define     MMC_REG_BIT10            (1 << 10)
#define     MMC_REG_BIT11            (1 << 11)
#define     MMC_REG_BIT12            (1 << 12)
#define     MMC_REG_BIT13            (1 << 13)
#define     MMC_REG_BIT14            (1 << 14)
#define     MMC_REG_BIT15            (1 << 15)
#define     MMC_REG_BIT16            (1 << 16)
#define     MMC_REG_BIT17            (1 << 17)
#define     MMC_REG_BIT18            (1 << 18)
#define     MMC_REG_BIT19            (1 << 19)
#define     MMC_REG_BIT20            (1 << 20)
#define     MMC_REG_BIT21            (1 << 21)
#define     MMC_REG_BIT22            (1 << 22)
#define     MMC_REG_BIT23            (1 << 23)
#define     MMC_REG_BIT24            (1 << 24)
#define     MMC_REG_BIT25            (1 << 25)
#define     MMC_REG_BIT26            (1 << 26)
#define     MMC_REG_BIT27            (1 << 27)
#define     MMC_REG_BIT28            (1 << 28)
#define     MMC_REG_BIT29            (1 << 29)
#define     MMC_REG_BIT30            (1 << 30)
#define     MMC_REG_BIT31            (1 << 31)
/*****************************************************************************/
/* Data Types                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/
typedef enum
{
    MEDIA_CONNECTED    = 0x1, /* First chunk */
    MEDIA_DISCONNECTED = 0x2  /* Last chunk  */
}ASYNC_EVENT;

typedef enum
{
    sd_card,
    sdhc_card,
    sdxc_card,
    mmc_card_4_2,
    mmc_card_4_4,
    mmc_card_4_5,
    mmc_card_5_0
} CARD_TYPE_T;

#define CARD_MODE mmc_card_5_0
/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/
struct udevice;

typedef struct mmc_struct
{
    u32 block_len;
    u32 block_cnt;
    u32 read_blk_num; /* the number of block data that send to master */
    u32 rx_blk_num;
    u32 rw_buf;
    u32 cmd_argu;
}mmc_struct_t;

/*****************************************************************************/
/* Extern variable declarations                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/
mmc_struct_t *get_mmc_info(void);
void update_cid_val(void);
void update_csd_val(void);
void x2_bifsd_isr(void);
void x2_bifsd_init(struct udevice *dev);
extern void x2_bifsd_initialize(void);
/*****************************************************************************/
/* INLINE Functions                                                          */
/*****************************************************************************/
static inline void bifsd_writel(u32 val, void *addr)
{
    writel(val, addr);
}
static inline u32 bifsd_readl(void *addr)
{
    return readl(addr);
}

static inline void bifsd_reset(void)
{
    bifsd_writel(0x20, (void *)SYSCTRL_BASE + 0x400);
    udelay(10);
    bifsd_writel(0, (void *)SYSCTRL_BASE + 0x400);
}
/* This function enables mmc interrupt 1 */
static inline void enable_mmc_int_1(void)
{
#ifdef BIFSD_ACC_ENABLE
    u32 reg_val = 0xffffcfff;
#else
    u32 reg_val = 0xffffffff;
#endif

    bifsd_writel(reg_val, rMMC_INT_ENABLE_1);
}
/* This function enables mmc interrupt 2 */
static inline void enable_mmc_int_2(void)
{
    u32 reg_val = 0x007fffff;

    bifsd_writel(reg_val, rMMC_INT_ENABLE_2);
}
/* This function config power up register */

static inline void mmc_set_power_up(CARD_TYPE_T mmc_type)
{
    u32 reg_val = 0;
    if((mmc_type == sd_card)||(mmc_type == sdhc_card)||(mmc_type == sdxc_card))
    {
        reg_val |= MMC_REG_BIT00;
    }
    else
    {
        reg_val |= MMC_REG_BIT02;
    }
    bifsd_writel(reg_val, rMMC_POWER_UP);
}
/* This function Set hardware reset count
   that the number of AHB clock cycles equivalent to 1Microseconds */

static inline void mmc_set_hard_reset_cnt(void)
{
    u32 pwr_cnt_val = 0;

    pwr_cnt_val = MMC_CPU_CLOCK/1000000;
    if(pwr_cnt_val == 0)
        pwr_cnt_val = 1;
    bifsd_writel(pwr_cnt_val, rMMC_HARD_RESET_CNT);
}

/*OCR value - 0xff8000(standard capacity card);
  0x40ff8000(high capacity card);
  0xff8080(dual voltage card);
  0x41ff8000(Extended capacity card)*/
static inline void mmc_config_ocr_reg(CARD_TYPE_T mmc_type)
{
    u32 reg_val = 0;
    if((mmc_type == sd_card)||(mmc_type == sdhc_card)||(mmc_type == sdxc_card))
    {
        reg_val = 0x40ff8000;
    }
    else
    {
        reg_val = 0x40000080;
    }

    bifsd_writel(reg_val, rMMC_OCR);
}

static inline void mmc_set_out_range_addr(void)
{
    bifsd_writel(OUT_RANGE_ADDR, rMMC_OUT_RANGE_ADDR);
}

static inline void enable_sd_security_interrupt(void)
{
    bifsd_writel(SD_SECURITY_INT_ENABLE_VAL, rMMC_SD_SECURITY_INT_ENABLE);
}
static inline void set_security_block_count(void)
{
    u32 reg_val = 0;

    reg_val &=0xFFFF0000;
    reg_val |= MMC_REG_BIT31;
    bifsd_writel(reg_val, rMMC_BLOCK_COUNT_SECURITY);
}
static inline void card_power_up(void)
{
    u32 reg_val = 0;

    reg_val = bifsd_readl(rMMC_OCR);
    reg_val |= MMC_REG_BIT31;
    bifsd_writel(reg_val, rMMC_OCR);
}
/* This function returns MMC Slave interruptstatus */
static inline u32 get_mmc_int1_status(void)
{
    return bifsd_readl(rMMC_INT_STATUS_1);
}
/* This function returns MMC Slave interruptstatus */
static inline u32 get_mmc_int2_status(void)
{
    return bifsd_readl(rMMC_INT_STATUS_2);
}
/* GPIO PIN MUX CONFIG */

static inline void bifsd_pin_mux_config(void)
{
    u32 reg_val;

    reg_val = bifsd_readl(GPIO1_CFG);
    reg_val &= 0xFFC00000;
    bifsd_writel(reg_val, GPIO1_CFG);
}

static inline void mmc_disable_acc_bypass(void)
{
    u32 reg_val;
    reg_val = bifsd_readl((void *)ACC_BYPASS_BASE + 0x590);
    reg_val &= 0xFFFFFFFE;
    bifsd_writel(reg_val, (void *)ACC_BYPASS_BASE + 0x590);
}
static inline void bifsd_config_timing(void)
{
    u32 reg_val;

    reg_val = (NAC_NON_HS_200_400_VAL | (NAC_HS_200_400_VAL << 4) | (NCR_VALUE << 8) |  (NCRC_VALUE << 16));
    bifsd_writel(reg_val, rMMC_TIMING);
}
#endif /* X2_BIFSD_H */



