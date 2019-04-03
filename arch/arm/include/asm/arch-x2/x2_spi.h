/*
 * Copyright (C) 2018/04/27 Horizon Robotics Co., Ltd.
 *
 * x2_spi.h
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef   __X2_SPI__
#define   __X2_SPI__

/*************************************************************
 * base register
*************************************************************/

#define   X2_SPI_BASE_ADDR_0                   (0xA5004000)
#define   X2_SPI_REG_0(offset)                 (X2_SPI_BASE_ADDR_0 | offset)
#define   X2_SPI_BASE_ADDR_1                   (0xA5005000)
#define   X2_SPI_REG_1(offset)                 (X2_SPI_BASE_ADDR_1 | offset)
#define   X2_SPI_BASE_ADDR_2                   (0xA5006000)
#define   X2_SPI_REG_2(offset)                 (X2_SPI_BASE_ADDR_2 | offset)


/*************************************************************
 * register list
*************************************************************/

#define   X2_SPI_TX                    (0x0)
#define   X2_SPI_RX                    (0x4)
#define   X2_SPI_CTRL                    (0x8)
#define   X2_SPI_SS                    (0xC)
#define   X2_SPI_SFSR                    (0x10)
#define   X2_SPI_RFTO_OFF                    (0x14)
#define   X2_SPI_TLEN_OFF                    (0x18)
#define   X2_SPI_INSTRUCT_OFF                    (0x1C)
#define   X2_SPI_INST_MASK_OFF                    (0x20)
#define   X2_SPI_SPI_SRCPND_OFF                    (0x24)
#define   X2_SPI_SPI_INTMASK                    (0x28)
#define   X2_SPI_SPI_INTSETMASK                    (0x2C)
#define   X2_SPI_SPI_INTUNMASK                    (0x30)
#define   X2_SPI_DMA_CTRL0                    (0x34)
#define   X2_SPI_DMA_CTRL1                    (0x38)
#define   X2_SPI_TX_DMA_ADDR0                    (0x3C)
#define   X2_SPI_TX_DMA_SIZE0                    (0x40)
#define   X2_SPI_TX_DMA_ADDR1_OFF                    (0x44)
#define   X2_SPI_TX_DMA_SIZE1_OFF                    (0x48)
#define   X2_SPI_RX_DMA_ADDR0                    (0x4C)
#define   X2_SPI_RX_DMA_SIZE0                    (0x50)
#define   X2_SPI_RX_DMA_ADDR1_OFF                    (0x54)
#define   X2_SPI_RX_DMA_SIZE1_OFF                    (0x58)
#define   X2_SPI_TX_SIZE_OFF                    (0x5C)
#define   X2_SPI_RX_SIZE_OFF                    (0x60)
#define   X2_SPI_FIFO_RESET                    (0x64)
#define   X2_SPI_TX_DMA_SIZE_                    (0x68)
#define   X2_SPI_RX_DMA_SIZE_                    (0x6C)


/*************************************************************
 * register bit
*************************************************************/

/*    X2_SPI_TX    */
#define   X2_SPI_TX_WO(n)                (((n) & 0xffff) << 0x0)

/*    X2_SPI_RX    */
#define   X2_SPI_RX_RO                (0xffff << 0x0)
#define   X2_SPI_RX_RO_SHIT(n)                (((n) & 0xffff) >> 0x0)

/*    X2_SPI_CTRL    */
#define   X2_SPI_SLAVE_SEL_HI(n)                (((n) & 0x1) << 0x1f)
#define   X2_SPI_SLAVE_SEL_HI_MASK                (0x1 << 0x1f)
#define   X2_SPI_SLAVE_SEL_HI_SHIT(n)                (((n) & 0x1) >> 0x1f)
#define   X2_SPI_SLAVE_SEL(n)                (((n) & 0x1) << 0x1e)
#define   X2_SPI_SLAVE_SEL_MASK                (0x1 << 0x1e)
#define   X2_SPI_SLAVE_SEL_SHIT(n)                (((n) & 0x1) >> 0x1e)
#define   X2_SPI_SAMP_CNT(n)                (((n) & 0x3) << 0x1c)
#define   X2_SPI_SAMP_CNT_MASK                (0x3 << 0x1c)
#define   X2_SPI_SAMP_CNT_SHIT(n)                (((n) & 0x3) >> 0x1c)
#define   X2_SPI_SAMP_SEL(n)                (((n) & 0x1) << 0x1b)
#define   X2_SPI_SAMP_SEL_MASK                (0x1 << 0x1b)
#define   X2_SPI_SAMP_SEL_SHIT(n)                (((n) & 0x1) >> 0x1b)
#define   X2_SPI_DW_SEL(n)                (((n) & 0x1) << 0x1a)
#define   X2_SPI_DW_SEL_MASK                (0x1 << 0x1a)
#define   X2_SPI_DW_SEL_SHIT(n)                (((n) & 0x1) >> 0x1a)
#define   X2_SPI_TX_FIFOE(n)                (((n) & 0x1) << 0x19)
#define   X2_SPI_TX_FIFOE_MASK                (0x1 << 0x19)
#define   X2_SPI_TX_FIFOE_SHIT(n)                (((n) & 0x1) >> 0x19)
#define   X2_SPI_RX_FIFOE(n)                (((n) & 0x1) << 0x18)
#define   X2_SPI_RX_FIFOE_MASK                (0x1 << 0x18)
#define   X2_SPI_RX_FIFOE_SHIT(n)                (((n) & 0x1) >> 0x18)
#define   X2_SPI_TX_DIS(n)                (((n) & 0x1) << 0x17)
#define   X2_SPI_TX_DIS_MASK                (0x1 << 0x17)
#define   X2_SPI_TX_DIS_SHIT(n)                (((n) & 0x1) >> 0x17)
#define   X2_SPI_RX_DIS(n)                (((n) & 0x1) << 0x16)
#define   X2_SPI_RX_DIS_MASK                (0x1 << 0x16)
#define   X2_SPI_RX_DIS_SHIT(n)                (((n) & 0x1) >> 0x16)
#define   X2_SPI_MS_MODE(n)                (((n) & 0x1) << 0x15)
#define   X2_SPI_MS_MODE_MASK                (0x1 << 0x15)
#define   X2_SPI_MS_MODE_SHIT(n)                (((n) & 0x1) >> 0x15)
#define   X2_SPI_PHASE(n)                (((n) & 0x1) << 0x14)
#define   X2_SPI_PHASE_MASK                (0x1 << 0x14)
#define   X2_SPI_PHASE_SHIT(n)                (((n) & 0x1) >> 0x14)
#define   X2_SPI_POLARITY(n)                (((n) & 0x1) << 0x13)
#define   X2_SPI_POLARITY_MASK                (0x1 << 0x13)
#define   X2_SPI_POLARITY_SHIT(n)                (((n) & 0x1) >> 0x13)
#define   X2_SPI_LSB(n)                (((n) & 0x1) << 0x12)
#define   X2_SPI_LSB_MASK                (0x1 << 0x12)
#define   X2_SPI_LSB_SHIT(n)                (((n) & 0x1) >> 0x12)
#define   X2_SPI_SSAL(n)                (((n) & 0x1) << 0x11)
#define   X2_SPI_SSAL_MASK                (0x1 << 0x11)
#define   X2_SPI_SSAL_SHIT(n)                (((n) & 0x1) >> 0x11)
#define   X2_SPI_SPI_EN(n)                (((n) & 0x1) << 0x10)
#define   X2_SPI_SPI_EN_MASK                (0x1 << 0x10)
#define   X2_SPI_SPI_EN_SHIT(n)                (((n) & 0x1) >> 0x10)
#define   X2_SPI_DIVIDER(n)                (((n) & 0xffff) << 0x0)
#define   X2_SPI_DIVIDER_MASK                (0xffff << 0x0)
#define   X2_SPI_DIVIDER_SHIT(n)                (((n) & 0xffff) >> 0x0)

/*    X2_SPI_SS    */
#define   X2_SPI_PRE_LIMIT(n)                (((n) & 0xf) << 0x8)
#define   X2_SPI_PRE_LIMIT_MASK                (0xf << 0x8)
#define   X2_SPI_PRE_LIMIT_SHIT(n)                (((n) & 0xf) >> 0x8)
#define   X2_SPI_POS_LIMIT(n)                (((n) & 0xf) << 0x4)
#define   X2_SPI_POS_LIMIT_MASK                (0xf << 0x4)
#define   X2_SPI_POS_LIMIT_SHIT(n)                (((n) & 0xf) >> 0x4)
#define   X2_SPI_SSH_LIMIT(n)                (((n) & 0xf) << 0x0)
#define   X2_SPI_SSH_LIMIT_MASK                (0xf << 0x0)
#define   X2_SPI_SSH_LIMIT_SHIT(n)                (((n) & 0xf) >> 0x0)

/*    X2_SPI_SFSR    */
#define   X2_SPI_TX_RST_DONE_RO                (0x1 << 0x11)
#define   X2_SPI_TX_RST_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0x11)
#define   X2_SPI_RX_RST_DONE_RO                (0x1 << 0x10)
#define   X2_SPI_RX_RST_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0x10)
#define   X2_SPI_TX_QUE_DONE_RO                (0x1 << 0xf)
#define   X2_SPI_TX_QUE_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0xf)
#define   X2_SPI_TX_TER_DONE_RO                (0x1 << 0xe)
#define   X2_SPI_TX_TER_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0xe)
#define   X2_SPI_RX_QUE_DONE_RO                (0x1 << 0xd)
#define   X2_SPI_RX_QUE_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0xd)
#define   X2_SPI_RX_FLUSH_DONE_RO                (0x1 << 0xc)
#define   X2_SPI_RX_FLUSH_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0xc)
#define   X2_SPI_RX_TER_DONE_RO                (0x1 << 0xb)
#define   X2_SPI_RX_TER_DONE_RO_SHIT(n)                (((n) & 0x1) >> 0xb)
#define   X2_SPI_TX_BG_INDEX_RO                (0x1 << 0xa)
#define   X2_SPI_TX_BG_INDEX_RO_SHIT(n)                (((n) & 0x1) >> 0xa)
#define   X2_SPI_RX_BG_INDEX_RO                (0x1 << 0x9)
#define   X2_SPI_RX_BG_INDEX_RO_SHIT(n)                (((n) & 0x1) >> 0x9)
#define   X2_SPI_TIP_RO                (0x1 << 0x8)
#define   X2_SPI_TIP_RO_SHIT(n)                (((n) & 0x1) >> 0x8)
#define   X2_SPI_TXDON_RO                (0x1 << 0x7)
#define   X2_SPI_TXDON_RO_SHIT(n)                (((n) & 0x1) >> 0x7)
#define   X2_SPI_RXDON_RO                (0x1 << 0x6)
#define   X2_SPI_RXDON_RO_SHIT(n)                (((n) & 0x1) >> 0x6)
#define   X2_SPI_TF_EMPTY_RO                (0x1 << 0x5)
#define   X2_SPI_TF_EMPTY_RO_SHIT(n)                (((n) & 0x1) >> 0x5)
#define   X2_SPI_DATA_RDY_RO                (0x1 << 0x4)
#define   X2_SPI_DATA_RDY_RO_SHIT(n)                (((n) & 0x1) >> 0x4)

/*    X2_SPI_RFTO    */
#define   X2_SPI_RFTO(n)                (((n) & 0x1ffff) << 0x0)
#define   X2_SPI_RFTO_MASK                (0x1ffff << 0x0)
#define   X2_SPI_RFTO_SHIT(n)                (((n) & 0x1ffff) >> 0x0)

/*    X2_SPI_TLEN    */
#define   X2_SPI_TLEN(n)                (((n) & 0xffffff) << 0x0)
#define   X2_SPI_TLEN_MASK                (0xffffff << 0x0)
#define   X2_SPI_TLEN_SHIT(n)                (((n) & 0xffffff) >> 0x0)

/*    X2_SPI_INSTRUCT    */
#define   X2_SPI_INSTRUCT(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_INSTRUCT_MASK                (0xffffffff << 0x0)
#define   X2_SPI_INSTRUCT_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_INST_MASK    */
#define   X2_SPI_INST_MASK(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_INST_MASK_MASK                (0xffffffff << 0x0)
#define   X2_SPI_INST_MASK_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_SPI_SRCPND    */
#define   X2_SPI_DMA_TRDONE_W1C                (0x1 << 0xa)
#define   X2_SPI_TX_BGDONE_W1C                (0x1 << 0x9)
#define   X2_SPI_RX_BGDONE_W1C                (0x1 << 0x8)
#define   X2_SPI_TX_EMPTY_W1C                (0x1 << 0x7)
#define   X2_SPI_RX_FULL_W1C                (0x1 << 0x6)
#define   X2_SPI_RX_TIMEOUT_W1C                (0x1 << 0x5)
#define   X2_SPI_TX_DMA_ERR_W1C                (0x1 << 0x4)
#define   X2_SPI_RX_DMA_ERR_W1C                (0x1 << 0x3)
#define   X2_SPI_TEMPTY_W1C                (0x1 << 0x2)
#define   X2_SPI_OE_W1C                (0x1 << 0x1)
#define   X2_SPI_DR_W1C                (0x1 << 0x0)
#define X2_SPI_CLR_ALL_PENDING_INT	\
(X2_SPI_DMA_TRDONE_W1C | X2_SPI_TX_BGDONE_W1C | X2_SPI_RX_BGDONE_W1C | \
 X2_SPI_TX_EMPTY_W1C | X2_SPI_RX_FULL_W1C | X2_SPI_RX_TIMEOUT_W1C | \
 X2_SPI_TX_DMA_ERR_W1C | X2_SPI_RX_DMA_ERR_W1C | X2_SPI_TEMPTY_W1C | \
 X2_SPI_OE_W1C | X2_SPI_DR_W1C)

/*    X2_SPI_SPI_INTMASK    */
#define   X2_SPI_DMA_TRDONE_RO                (0x1 << 0xa)
#define   X2_SPI_DMA_TRDONE_RO_SHIT(n)                (((n) & 0x1) >> 0xa)
#define   X2_SPI_TX_BGDONE_RO                (0x1 << 0x9)
#define   X2_SPI_TX_BGDONE_RO_SHIT(n)                (((n) & 0x1) >> 0x9)
#define   X2_SPI_RX_BGDONE_RO                (0x1 << 0x8)
#define   X2_SPI_RX_BGDONE_RO_SHIT(n)                (((n) & 0x1) >> 0x8)
#define   X2_SPI_TX_EMPTY_RO                (0x1 << 0x7)
#define   X2_SPI_TX_EMPTY_RO_SHIT(n)                (((n) & 0x1) >> 0x7)
#define   X2_SPI_RX_FULL_RO                (0x1 << 0x6)
#define   X2_SPI_RX_FULL_RO_SHIT(n)                (((n) & 0x1) >> 0x6)
#define   X2_SPI_RX_TIMEOUT_RO                (0x1 << 0x5)
#define   X2_SPI_RX_TIMEOUT_RO_SHIT(n)                (((n) & 0x1) >> 0x5)
#define   X2_SPI_TX_DMA_ERR_RO                (0x1 << 0x4)
#define   X2_SPI_TX_DMA_ERR_RO_SHIT(n)                (((n) & 0x1) >> 0x4)
#define   X2_SPI_RX_DMA_ERR_RO                (0x1 << 0x3)
#define   X2_SPI_RX_DMA_ERR_RO_SHIT(n)                (((n) & 0x1) >> 0x3)
#define   X2_SPI_TEMPTY_RO                (0x1 << 0x2)
#define   X2_SPI_TEMPTY_RO_SHIT(n)                (((n) & 0x1) >> 0x2)
#define   X2_SPI_OE_RO                (0x1 << 0x1)
#define   X2_SPI_OE_RO_SHIT(n)                (((n) & 0x1) >> 0x1)
#define   X2_SPI_DR_RO                (0x1 << 0x0)
#define   X2_SPI_DR_RO_SHIT(n)                (((n) & 0x1) >> 0x0)

/*    X2_SPI_SPI_INTSETMASK    */
#define   X2_SPI_DMA_TRDONE_WO(n)                (((n) & 0x1) << 0xa)
#define   X2_SPI_TX_BGDONE_WO(n)                (((n) & 0x1) << 0x9)
#define   X2_SPI_RX_BGDONE_WO(n)                (((n) & 0x1) << 0x8)
#define   X2_SPI_TX_EMPTY_WO(n)                (((n) & 0x1) << 0x7)
#define   X2_SPI_RX_FULL_WO(n)                (((n) & 0x1) << 0x6)
#define   X2_SPI_RX_TIMEOUT_WO(n)                (((n) & 0x1) << 0x5)
#define   X2_SPI_TX_DMA_ERR_WO(n)                (((n) & 0x1) << 0x4)
#define   X2_SPI_RX_DMA_ERR_WO(n)                (((n) & 0x1) << 0x3)
#define   X2_SPI_TEMPTY_WO(n)                (((n) & 0x1) << 0x2)
#define   X2_SPI_OE_WO(n)                (((n) & 0x1) << 0x1)
#define   X2_SPI_DR_WO(n)                (((n) & 0x1) << 0x0)

/*    X2_SPI_SPI_INTUNMASK    */
#define   X2_SPI_DMA_TRDONE_WO(n)                (((n) & 0x1) << 0xa)
#define   X2_SPI_TX_BGDONE_WO(n)                (((n) & 0x1) << 0x9)
#define   X2_SPI_RX_BGDONE_WO(n)                (((n) & 0x1) << 0x8)
#define   X2_SPI_TX_EMPTY_WO(n)                (((n) & 0x1) << 0x7)
#define   X2_SPI_RX_FULL_WO(n)                (((n) & 0x1) << 0x6)
#define   X2_SPI_RX_TIMEOUT_WO(n)                (((n) & 0x1) << 0x5)
#define   X2_SPI_TX_DMA_ERR_WO(n)                (((n) & 0x1) << 0x4)
#define   X2_SPI_RX_DMA_ERR_WO(n)                (((n) & 0x1) << 0x3)
#define   X2_SPI_TEMPTY_WO(n)                (((n) & 0x1) << 0x2)
#define   X2_SPI_OE_WO(n)                (((n) & 0x1) << 0x1)
#define   X2_SPI_DR_WO(n)                (((n) & 0x1) << 0x0)

/*    X2_SPI_DMA_CTRL0    */
#define   X2_SPI_TX_APBSEL(n)                (((n) & 0x1) << 0xf)
#define   X2_SPI_TX_APBSEL_MASK                (0x1 << 0xf)
#define   X2_SPI_TX_APBSEL_SHIT(n)                (((n) & 0x1) >> 0xf)
#define   X2_SPI_TX_BG(n)                (((n) & 0x1) << 0xe)
#define   X2_SPI_TX_BG_MASK                (0x1 << 0xe)
#define   X2_SPI_TX_BG_SHIT(n)                (((n) & 0x1) >> 0xe)
#define   X2_SPI_TX_MAXOS(n)                (((n) & 0x3) << 0xc)
#define   X2_SPI_TX_MAXOS_MASK                (0x3 << 0xc)
#define   X2_SPI_TX_MAXOS_SHIT(n)                (((n) & 0x3) >> 0xc)
#define   X2_SPI_TX_AL(n)                (((n) & 0x1) << 0xb)
#define   X2_SPI_TX_AL_MASK                (0x1 << 0xb)
#define   X2_SPI_TX_AL_SHIT(n)                (((n) & 0x1) >> 0xb)
#define   X2_SPI_TX_BURST_LEN(n)                (((n) & 0x3) << 0x9)
#define   X2_SPI_TX_BURST_LEN_MASK                (0x3 << 0x9)
#define   X2_SPI_TX_BURST_LEN_SHIT(n)                (((n) & 0x3) >> 0x9)
#define   X2_SPI_RX_APBSEL(n)                (((n) & 0x1) << 0x6)
#define   X2_SPI_RX_APBSEL_MASK                (0x1 << 0x6)
#define   X2_SPI_RX_APBSEL_SHIT(n)                (((n) & 0x1) >> 0x6)
#define   X2_SPI_RX_BG(n)                (((n) & 0x1) << 0x5)
#define   X2_SPI_RX_BG_MASK                (0x1 << 0x5)
#define   X2_SPI_RX_BG_SHIT(n)                (((n) & 0x1) >> 0x5)
#define   X2_SPI_RX_MAXOS(n)                (((n) & 0x3) << 0x3)
#define   X2_SPI_RX_MAXOS_MASK                (0x3 << 0x3)
#define   X2_SPI_RX_MAXOS_SHIT(n)                (((n) & 0x3) >> 0x3)
#define   X2_SPI_RX_AL(n)                (((n) & 0x1) << 0x2)
#define   X2_SPI_RX_AL_MASK                (0x1 << 0x2)
#define   X2_SPI_RX_AL_SHIT(n)                (((n) & 0x1) >> 0x2)
#define   X2_SPI_RX_BURST_LEN(n)                (((n) & 0x3) << 0x0)
#define   X2_SPI_RX_BURST_LEN_MASK                (0x3 << 0x0)
#define   X2_SPI_RX_BURST_LEN_SHIT(n)                (((n) & 0x3) >> 0x0)

/*    X2_SPI_DMA_CTRL1    */
#define   X2_SPI_TX_DMA_ABORT(n)                (((n) & 0x1) << 0x8)
#define   X2_SPI_TX_DMA_ABORT_MASK                (0x1 << 0x8)
#define   X2_SPI_TX_DMA_ABORT_SHIT(n)                (((n) & 0x1) >> 0x8)
#define   X2_SPI_TX_DMA_STOP(n)                (((n) & 0x1) << 0x7)
#define   X2_SPI_TX_DMA_STOP_MASK                (0x1 << 0x7)
#define   X2_SPI_TX_DMA_STOP_SHIT(n)                (((n) & 0x1) >> 0x7)
#define   X2_SPI_TX_DMA_CFG(n)                (((n) & 0x1) << 0x6)
#define   X2_SPI_TX_DMA_CFG_MASK                (0x1 << 0x6)
#define   X2_SPI_TX_DMA_CFG_SHIT(n)                (((n) & 0x1) >> 0x6)
#define   X2_SPI_TX_DMA_START(n)                (((n) & 0x1) << 0x5)
#define   X2_SPI_TX_DMA_START_MASK                (0x1 << 0x5)
#define   X2_SPI_TX_DMA_START_SHIT(n)                (((n) & 0x1) >> 0x5)
#define   X2_SPI_RX_DMA_FLUSH(n)                (((n) & 0x1) << 0x4)
#define   X2_SPI_RX_DMA_FLUSH_MASK                (0x1 << 0x4)
#define   X2_SPI_RX_DMA_FLUSH_SHIT(n)                (((n) & 0x1) >> 0x4)
#define   X2_SPI_RX_DMA_ABORT(n)                (((n) & 0x1) << 0x3)
#define   X2_SPI_RX_DMA_ABORT_MASK                (0x1 << 0x3)
#define   X2_SPI_RX_DMA_ABORT_SHIT(n)                (((n) & 0x1) >> 0x3)
#define   X2_SPI_RX_DMA_STOP(n)                (((n) & 0x1) << 0x2)
#define   X2_SPI_RX_DMA_STOP_MASK                (0x1 << 0x2)
#define   X2_SPI_RX_DMA_STOP_SHIT(n)                (((n) & 0x1) >> 0x2)
#define   X2_SPI_RX_DMA_CFG(n)                (((n) & 0x1) << 0x1)
#define   X2_SPI_RX_DMA_CFG_MASK                (0x1 << 0x1)
#define   X2_SPI_RX_DMA_CFG_SHIT(n)                (((n) & 0x1) >> 0x1)
#define   X2_SPI_RX_DMA_START(n)                (((n) & 0x1) << 0x0)
#define   X2_SPI_RX_DMA_START_MASK                (0x1 << 0x0)
#define   X2_SPI_RX_DMA_START_SHIT(n)                (((n) & 0x1) >> 0x0)

/*    X2_SPI_TX_DMA_ADDR0    */
#define   X2_SPI_TX_DMA_ADDR(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_TX_DMA_ADDR_MASK                (0xffffffff << 0x0)
#define   X2_SPI_TX_DMA_ADDR_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_TX_DMA_SIZE0    */
#define   X2_SPI_TX_DMA_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_TX_DMA_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_TX_DMA_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_TX_DMA_ADDR1    */
#define   X2_SPI_TX_DMA_ADDR1(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_TX_DMA_ADDR1_MASK                (0xffffffff << 0x0)
#define   X2_SPI_TX_DMA_ADDR1_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_TX_DMA_SIZE1    */
#define   X2_SPI_TX_DMA_SIZE1(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_TX_DMA_SIZE1_MASK                (0x1fffff << 0x0)
#define   X2_SPI_TX_DMA_SIZE1_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_RX_DMA_ADDR0    */
#define   X2_SPI_RX_DMA_ADDR(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_RX_DMA_ADDR_MASK                (0xffffffff << 0x0)
#define   X2_SPI_RX_DMA_ADDR_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_RX_DMA_SIZE0    */
#define   X2_SPI_RX_DMA_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_RX_DMA_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_RX_DMA_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_RX_DMA_ADDR1    */
#define   X2_SPI_RX_DMA_ADDR1(n)                (((n) & 0xffffffff) << 0x0)
#define   X2_SPI_RX_DMA_ADDR1_MASK                (0xffffffff << 0x0)
#define   X2_SPI_RX_DMA_ADDR1_SHIT(n)                (((n) & 0xffffffff) >> 0x0)

/*    X2_SPI_RX_DMA_SIZE1    */
#define   X2_SPI_RX_DMA_SIZE1(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_RX_DMA_SIZE1_MASK                (0x1fffff << 0x0)
#define   X2_SPI_RX_DMA_SIZE1_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_TX_SIZE    */
#define   X2_SPI_TX_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_TX_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_TX_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_RX_SIZE    */
#define   X2_SPI_RX_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_RX_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_RX_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_FIFO_RESET    */
#define   X2_SPI_SPI_ABORT(n)                (((n) & 0x1) << 0x8)
#define   X2_SPI_SPI_ABORT_MASK                (0x1 << 0x8)
#define   X2_SPI_SPI_ABORT_SHIT(n)                (((n) & 0x1) >> 0x8)
#define   X2_SPI_TX_FIFOINT_DIS(n)                (((n) & 0x1) << 0x7)
#define   X2_SPI_TX_FIFOINT_DIS_MASK                (0x1 << 0x7)
#define   X2_SPI_TX_FIFOINT_DIS_SHIT(n)                (((n) & 0x1) >> 0x7)
#define   X2_SPI_RX_FIFOINT_DIS(n)                (((n) & 0x1) << 0x6)
#define   X2_SPI_RX_FIFOINT_DIS_MASK                (0x1 << 0x6)
#define   X2_SPI_RX_FIFOINT_DIS_SHIT(n)                (((n) & 0x1) >> 0x6)
#define   X2_SPI_RX_QUERY_EN(n)                (((n) & 0x1) << 0x5)
#define   X2_SPI_RX_QUERY_EN_MASK                (0x1 << 0x5)
#define   X2_SPI_RX_QUERY_EN_SHIT(n)                (((n) & 0x1) >> 0x5)
#define   X2_SPI_TX_QUERY_EN(n)                (((n) & 0x1) << 0x4)
#define   X2_SPI_TX_QUERY_EN_MASK                (0x1 << 0x4)
#define   X2_SPI_TX_QUERY_EN_SHIT(n)                (((n) & 0x1) >> 0x4)
#define   X2_SPI_RXFIFO_SCLEAR(n)                (((n) & 0x1) << 0x3)
#define   X2_SPI_RXFIFO_SCLEAR_MASK                (0x1 << 0x3)
#define   X2_SPI_RXFIFO_SCLEAR_SHIT(n)                (((n) & 0x1) >> 0x3)
#define   X2_SPI_TXFIFO_SCLEAR(n)                (((n) & 0x1) << 0x2)
#define   X2_SPI_TXFIFO_SCLEAR_MASK                (0x1 << 0x2)
#define   X2_SPI_TXFIFO_SCLEAR_SHIT(n)                (((n) & 0x1) >> 0x2)
#define   X2_SPI_RXFIFO_CLEAR(n)                (((n) & 0x1) << 0x1)
#define   X2_SPI_RXFIFO_CLEAR_MASK                (0x1 << 0x1)
#define   X2_SPI_RXFIFO_CLEAR_SHIT(n)                (((n) & 0x1) >> 0x1)
#define   X2_SPI_TXFIFO_CLEAR(n)                (((n) & 0x1) << 0x0)
#define   X2_SPI_TXFIFO_CLEAR_MASK                (0x1 << 0x0)
#define   X2_SPI_TXFIFO_CLEAR_SHIT(n)                (((n) & 0x1) >> 0x0)

/*    X2_SPI_TX_DMA_SIZE_    */
#define   X2_SPI_TX_DMA_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_TX_DMA_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_TX_DMA_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

/*    X2_SPI_RX_DMA_SIZE_    */
#define   X2_SPI_RX_DMA_SIZE(n)                (((n) & 0x1fffff) << 0x0)
#define   X2_SPI_RX_DMA_SIZE_MASK                (0x1fffff << 0x0)
#define   X2_SPI_RX_DMA_SIZE_SHIT(n)                (((n) & 0x1fffff) >> 0x0)

#define BITS_8 8
#define BITS_16 16

#define DLY_SAMP_CLK_1	1
#define DLY_SAMP_CLK_2	2
#define DLY_SAMP_CLK_3	3
#define DLY_SAMP_CLK_4	4

#endif   /*__X2_SPI__*/

