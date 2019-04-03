#ifndef _X2_QSPI_H_
#define _X2_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Definition of SPI_CTRL1 */
#define X2_QSPI_LSB                     (0x40)
#define X2_QSPI_CPHA                    (0x20)
#define X2_QSPI_CPOL                    (0x10)
#define X2_QSPI_MST                     (0x04)

#define TX_ENABLE                       (0x08)
#define TX_DISABLE                      (0x00)
#define RX_ENABLE                       (0x80)
#define RX_DISABLE                      (0x00)

#define FIFO_WIDTH32                    (0x02)
#define FIFO_WIDTH16                    (0x01)
#define FIFO_WIDTH8                     (0x00)
#define FIFO_DEPTH                      (16)
#define BATCH_MAX_SIZE                  (0x10000)

#define SPI_FIFO_DEPTH                  (FIFO_DEPTH)
#define SPI_BACKUP_OFFSET               (0x10000) //64K
#define SPI_CS0                         (0x01)

/* Definition of SPI_CTRL2 */
#define BATCH_TINT_EN                   (0x1L<<7)
#define BATCH_RINT_EN                   (0x1L<<6)
#define MODDEF                          (0x1L<<3)
#define ERR_INT_EN                      (0x1l<<2)
#define TX_INT_EN                       (0x1L<<1)
#define RX_INT_EN                       (0x1L<<0)
/* Definition of SPI_CTRL3 */
#define BATCH_DISABLE                   (0x1L<<4)
#define BATCH_ENABLE                    (0x0L<<4)
#define FIFO_RESET                      (0x03)
#define SW_RST_TX                       (0x02)
#define SW_RST_RX                       (0x01)
#define RX_DMA_EN                       (0x04)
#define TX_DMA_EN                       (0x08)
/* STATUS1 */
#define BATCH_TXDONE                    (0x1L<<5)
#define BATCH_RXDONE                    (0x1L<<4)
#define MODF_CLR                        (0x1L<<3)
#define TX_ALMOST_EMPTY                 (0x1L<<1)
#define RX_ALMOST_FULL                  (0x1L<<0)
/* STATUS2 */
#define SSN_IN                          (0x1L<<7)
#define TXFIFO_FULL                     (0x1L<<5)
#define TXFIFO_EMPTY                    (0x1L<<1)
#define RXFIFO_EMPTY                    (0x1L<<4)
#define RXFIFO_FULL                     (0x1L<<0)
/* SPI_DUAL_QUAD_MODE */
#define HOLD_OUTPUT                     (0x1 << 7)
#define HOLD_OE                         (0x1 << 6)
#define HOLD_CTL                        (0x1 << 5)
#define WP_OUTPUT                       (0x1 << 4)
#define WP_OE                           (0x1 << 3)
#define WP_CTL                          (0x1 << 2)
#define QPI_ENABLE                      (0x1 << 1)
#define DPI_ENABLE                      (0x1 << 0)
#define SPI_XFER_TX                     0
#define SPI_XFER_RX                     1
#define SPI_XFER_T                      2
/* SPI_XIP_CFG */
#define XIP_EN                          (0x1L<<0)
#define FLASH_SW_CFG                    (0x0L<<1)
#define FLASH_HW_CFG                    (0x1L<<1)
#define FLASH_CTNU_MODE                 (0x1<<4)
#define DUMMY_CYCLE_0                   (0x0<<5)
#define DUMMY_CYCLE_1                   (0x1<<5)
#define DUMMY_CYCLE_2                   (0x2<<5)
#define DUMMY_CYCLE_3                   (0x3<<5)
#define DUMMY_CYCLE_4                   (0x4<<5)
#define DUMMY_CYCLE_5                   (0x5<<5)
#define DUMMY_CYCLE_6                   (0x6<<5)
#define DUMMY_CYCLE_7                   (0x7<<5)

#define XIP_FLASH_ADDR_OFFSET           0x100

/* xfer mode */
#define X2_QSPI_XFER_BYTE               0
#define X2_QSPI_XFER_BATCH              1
#define X2_QSPI_XFER_DMA                2

/* Read commands */
#define CMD_QUAD_PAGE_PROGRAM           0x32
#define CMD_READ_DUAL_OUTPUT_FAST       0x3b
#define CMD_READ_QUAD_OUTPUT_FAST       0x6b

#define MIN(a, b)   ((a < b) ? (a) : (b))
#define MAX(a, b)   ((a > b) ? (a) : (b))


#ifdef __cplusplus
}
#endif

#endif
