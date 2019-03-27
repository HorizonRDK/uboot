#ifndef __HOBOT_SPI_H__
#define __HOBOT_SPI_H__

#ifdef CONFIG_X2_NOR_BOOT

#define QSPI_DEV_CS0		        0x1
#define SPI_TXREG(_base_)			((_base_) + 0x0)
#define SPI_RXREG(_base_) 			((_base_) + 0x0)
#define SPI_SCLK(_base_) 		    ((_base_) + 0x4)
#define SPI_CTRL1(_base_)			((_base_) + 0x8)
#define SPI_CTRL2(_base_)			((_base_) + 0xC)
#define SPI_CTRL3(_base_)			((_base_) + 0x10)
#define SPI_CS(_base_)				((_base_) + 0x14)
#define SPI_STATUS1(_base_)			((_base_) + 0x18)
#define SPI_STATUS2(_base_)			((_base_) + 0x1C)
#define SPI_BATCH_CNT_RX(_base_)	((_base_) + 0x20)
#define SPI_BATCH_CNT_TX(_base_)	((_base_) + 0x24)
#define SPI_FIFO_RXTRIG_LVL(_base_)	((_base_) + 0x28)
#define SPI_FIFO_TXTRIG_LVL(_base_)	((_base_) + 0x2C)
#define SPI_DUAL_QUAD_MODE(_base_)	((_base_) + 0x30)
#define SPI_XIP_CFG(_base_)			((_base_) + 0x34)

#define MSB		 					(0x00)
#define CPHA_H						(0x20)
#define CPHA_L						(0x00)
#define CPOL_H						(0x10)
#define CPOL_L						(0x00)
#define MST_MODE					(0x04)

#define TX_ENABLE					(1 << 3)
#define RX_ENABLE					(1 << 7)

#define FIFO_WIDTH32				(0x02)
#define FIFO_WIDTH16				(0x01)
#define FIFO_WIDTH8					(0x00)
#define FIFO_DEPTH					(16)
#define BATCH_MAX_SIZE				(0x10000)

#define SPI_FIFO_DEPTH				(FIFO_DEPTH)
#define SPI_BACKUP_OFFSET			(0x10000)
#define SPI_CS0						(0x01)
#define SPI_MODE0 					(CPOL_L | CPHA_L)
#define SPI_MODE1 					(CPOL_L | CPHA_H)
#define SPI_MODE2 					(CPOL_H | CPHA_L)
#define SPI_MODE3 					(CPOL_H | CPHA_H)

#define SCLK_DIV(div, scaler)   	((2 << (div)) * (scaler + 1))
#define SCLK_VAL(div, scaler)		(((div) << 4) | ((scaler) << 0))

/* Definition of SPI_CTRL3 */
#define BATCH_DISABLE				(1 << 4)
#define TX_DMA_EN					(1 << 3)
#define RX_DMA_EN					(1 << 2)
#define FIFO_RESET					(SW_RST_TX | SW_RST_RX)
#define SW_RST_TX					(1 << 1)
#define SW_RST_RX					(1 << 0)

/* STATUS1 */
#define BATCH_TXDONE				(1 << 5)
#define BATCH_RXDONE				(1 << 4)
#define MODF_CLR					(1 << 3)
#define TX_ALMOST_EMPTY				(1 << 1)
#define RX_ALMOST_FULL				(1 << 0)

/* STATUS2 */
#define SSN_IN						(1 << 7)
#define TXFIFO_FULL					(1 << 5)
#define RXFIFO_EMPTY				(1 << 4)
#define TXFIFO_EMPTY				(1 << 1)
#define RXFIFO_FULL					(1 << 0)

/* SPI_DUAL_QUAD_MODE */
#define HOLD_OUTPUT					(1 << 7)
#define HOLD_OE						(1 << 6)
#define HOLD_CTL					(1 << 5)
#define WP_OUTPUT					(1 << 4)
#define WP_OE						(1 << 3)
#define WP_CTL						(1 << 2)
#define QPI_ENABLE					(1 << 1)
#define DPI_ENABLE					(1 << 0)
#define SPI_XFER_TX					0
#define SPI_XFER_RX					1
#define SPI_XFER_TXRX				2
/* SPI_XIP_CFG */
#define XIP_EN 						(1 << 0)
#define FLASH_HW_CFG				(1 << 1)
#define FLASH_CTNU_MODE				(1 << 4)
#define DUMMY_CYCLE_0				(0x0<<5)
#define DUMMY_CYCLE_1				(0x1<<5)
#define DUMMY_CYCLE_2				(0x2<<5)
#define DUMMY_CYCLE_3				(0x3<<5)
#define DUMMY_CYCLE_4				(0x4<<5)
#define DUMMY_CYCLE_5				(0x5<<5)
#define DUMMY_CYCLE_6				(0x6<<5)
#define DUMMY_CYCLE_7				(0x7<<5)

#define XIP_FLASH_ADDR_OFFSET		0x100
/* Batch mode or no batch mode */
#define SPI_BATCH_MODE				1
#define SPI_NO_BATCH_MODE			0

/* Wire mode */
#define SPI_N_MODE					1
#define SPI_D_MODE					2
#define SPI_Q_MODE					4

#define SPI_XFER_BEGIN		BIT(0)	/* Assert CS before transfer */
#define SPI_XFER_END		BIT(1)	/* Deassert CS after transfer */
#define SPI_XFER_ONCE		(SPI_XFER_BEGIN | SPI_XFER_END)
#define SPI_XFER_MMAP		BIT(2)	/* Memory Mapped start */
#define SPI_XFER_MMAP_END	BIT(3)	/* Memory Mapped End */
#define SPI_XFER_CMD        BIT(4)	/* Transfer data is CMD */

#define SPI_TX_BYTE	BIT(8)	/* transmit with 1 wire byte */
#define SPI_TX_DUAL	BIT(9)	/* transmit with 2 wires */
#define SPI_TX_QUAD	BIT(10)	/* transmit with 4 wires */
#define SPI_RX_SLOW	BIT(11)	/* receive with 1 wire slow */
#define SPI_RX_DUAL	BIT(12)	/* receive with 2 wires */
#define SPI_RX_QUAD	BIT(13)	/* receive with 4 wires */

/* Read commands */
#define CMD_READ_ARRAY_SLOW		0x03
#define CMD_READ_ARRAY_FAST		0x0b
#define CMD_READ_DUAL_OUTPUT_FAST	0x3b
#define CMD_READ_QUAD_OUTPUT_FAST	0x6b
#define CMD_READ_QUAD_IO_FAST		0xeb
#define CMD_READ_ID			0x9f
#define CMD_READ_STATUS			0x05
#define CMD_READ_CONFIG			0x35
#define CMD_FLAG_STATUS			0x70
#define CMD_RESET_EN	0x66
#define CMD_RESET		0x99

/* Erase commands */
#define CMD_ERASE_4K			0x20
#define CMD_ERASE_64K			0xd8

/* Write commands */
#define CMD_WRITE_STATUS		0x01
#define CMD_PAGE_PROGRAM		0x02
#define CMD_WRITE_ENABLE		0x06
#define CMD_QUAD_PAGE_PROGRAM		0x32

struct spi_slave {
	unsigned int max_hz;
	unsigned int speed;
	unsigned int bus;
	unsigned int cs;

	unsigned int mode;
	unsigned int wordlen;
	unsigned int max_write_size;
	void *memory_map;
	unsigned char option;
	unsigned char flags;
};

void spi_release_bus(struct spi_slave *slave);
void spi_claim_bus(struct spi_slave *slave);
struct spi_slave *spi_setup_slave(uint32_t spi_num, uint32_t sclk);
void spi_init(struct spi_slave *slave, uint32_t mclk, uint32_t sclk);
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags);
#endif
#endif
