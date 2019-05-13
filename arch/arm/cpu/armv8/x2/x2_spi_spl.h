#ifndef __HOBOT_SPI_H__
#define __HOBOT_SPI_H__

#if defined(CONFIG_X2_NOR_BOOT) || defined(CONFIG_X2_NAND_BOOT)

#define QSPI_DEV_CS0		        0x1
#define X2_QSPI_MCLK		(500000000)
#define X2_QSPI_SCLK		(50000000)

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

#endif
#endif
