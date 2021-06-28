/* SPDX-License-Identifier: GPL-2.0+
 *
 * QSPI driver
 *
 * Copyright (C) 2019, Horizon Robotics, <yang.lu@horizon.ai>
 */

#ifndef _HB_QSPI_H_
#define _HB_QSPI_H_

#define HB_QSPI_NAME        "hb_qspi"
#define HB_QSPI_MAX_CS      8
#define HB_QSPI_DEF_CS      0
#define HB_QSPI_DEF_BR      50000000
#define HB_QSPI_DEF_HCLK      500000000
/* X2 QSPI register offsets */
#define HB_QSPI_DAT_REG     0x00   /* Transmit/Receive data buffer */
#define HB_QSPI_BDR_REG     0x04   /* Master mode baud-rate control*/
#define HB_QSPI_CTL1_REG    0x08   /* SPI work mode configuration */
#define HB_QSPI_CTL2_REG    0x0C   /* SPI interrupt enabled */
#define HB_QSPI_CTL3_REG    0x10   /* SPI software reset and DMA enable */
#define HB_QSPI_CS_REG      0x14   /* Control the device select output */
#define HB_QSPI_ST1_REG     0x18   /* spi status reg1 */
#define HB_QSPI_ST2_REG     0x1C   /* spi status reg2 */
#define HB_QSPI_RBC_REG     0x20   /* Data number for RX batch transfer */
#define HB_QSPI_TBC_REG     0x24   /* Data number for TX batch transfer */
#define HB_QSPI_RTL_REG     0x28   /* RX FIFO trigger level register */
#define HB_QSPI_TTL_REG     0x2C   /* TX FIFO trigger level register */
#define HB_QSPI_DQM_REG     0x30   /* Dual, Quad command conrtol register */
#define HB_QSPI_XIP_REG     0x34   /* XIP config register */

/* Definition of SPI_CTRL1 */
#define HB_QSPI_FW_MASK     0x03   /* FIFO WIDTH MASK */
#define HB_QSPI_FW8         0x00   /* FIFO WIDTH 8Bit */
#define HB_QSPI_FW16        0x01   /* FIFO WIDTH 16Bit */
#define HB_QSPI_FW32        0x02   /* FIFO WIDTH 32Bit */
#define HB_QSPI_MST         0x04   /* Working mast mode */
#define HB_QSPI_TX_EN       0x08   /* Indicates the Tx direction is working */
#define HB_QSPI_CPOL        0x10   /* In idle state SCLK is high */
#define HB_QSPI_CPHA        0x20   /* Sampling of data at even edges */
#define HB_QSPI_LSB         0x40   /* LSB transferred first on SPI bus */
#define HB_QSPI_RX_EN       0x80   /* Indicates the Rx direction is working */

/* Definition of SPI_CTRL2 */
#define HB_QSPI_RX_INT      BIT(0) /* Enable the interrupt of RX */
#define HB_QSPI_TX_INT      BIT(1) /* Enable the interrupt of TX */
#define HB_QSPI_ERR_INT     BIT(2) /* Enable wr_full/rd_ept error interrupt */
#define HB_QSPI_MODF_INT    BIT(3) /* Enable mode fault detecting in ssn0_i */
#define HB_QSPI_RBC_INT     BIT(6) /* Enable spi rx batch complete interrupt */
#define HB_QSPI_TBC_INT     BIT(7) /* Enable spi tx batch complete interrupt */

/* Definition of SPI_CTRL3 */
#define HB_QSPI_RX_RST      BIT(0) /* soft reset for RX link, high active */
#define HB_QSPI_TX_RST      BIT(1) /* soft reset for TX link, high active */
#define HB_QSPI_RST_ALL     0x03   /* soft reset rx & tx link */
#define HB_QSPI_BATCH_DIS   BIT(4) /* Disable batch_cnt and batch operation */

/* Definition of SPI_STATUS1 */
#define HB_QSPI_RX_AF       BIT(0) /* Rx almost full */
#define HB_QSPI_TX_AE       BIT(1) /* Tx almost empty */
#define HB_QSPI_MODF        BIT(3) /* Work mode if fault */
#define HB_QSPI_RBD         BIT(4) /* Rx batch done */
#define HB_QSPI_TBD         BIT(5) /* Tx batch done */

/* Definition of SPI_STATUS2 */
#define HB_QSPI_RX_FULL     BIT(0)
#define HB_QSPI_TX_EP       BIT(1)
#define HB_QSPI_RXWR_FULL   BIT(2)
#define HB_QSPI_TXRD_EMPTY  BIT(3)
#define HB_QSPI_RX_EP       BIT(4)
#define HB_QSPI_TX_FULL     BIT(5)
#define HB_QSPI_SSN_IN      BIT(7)

/* Definition of RX/TX trig level */
#define HB_QSPI_FIFO_DEPTH  16
#define HB_QSPI_TRIG_LEVEL  (HB_QSPI_FIFO_DEPTH/2)

/* Definition of DUAL_QUAD_MODE */
#define HB_QSPI_DUAL        BIT(0)
#define HB_QSPI_QUAD        BIT(1)

/* xfer mode */
#define HB_QSPI_XFER_BYTE   0
#define HB_QSPI_XFER_BATCH  1
#define HB_QSPI_XFER_DMA    2

/* Read commands */
#define CMD_QUAD_PAGE_PROGRAM       0x32
#define CMD_QUAD_PAGE_PROGRAM_4A       0x34
#define CMD_READ_DUAL_OUTPUT_FAST   0x3b
#define CMD_READ_QUAD_OUTPUT_FAST   0x6b
#define CMD_READ_QUAD_OUTPUT_FAST_4A 0x6c

/* Some op macron */
#define HB_QSPI_OP_RX_EN            0x01
#define HB_QSPI_OP_RX_DIS           0x02
#define HB_QSPI_OP_TX_EN            0x03
#define HB_QSPI_OP_TX_DIS           0x04
#define HB_QSPI_OP_BAT_EN           0x05
#define HB_QSPI_OP_BAT_DIS          0x06

#define TRYS_TOTAL_NUM              0x10000
#define BATCH_MAX_CNT               0x10000
#define MIN(a, b)   ((a < b) ? (a) : (b))
#define MAX(a, b)   ((a > b) ? (a) : (b))

#endif
