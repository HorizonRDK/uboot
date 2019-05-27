#ifndef __HOBOT_NOR_FLASH_H__
#define __HOBOT_NOR_FLASH_H__

#ifdef CONFIG_X2_NOR_BOOT

#define SPI_FLASH_MAX_ID_LEN	8

void x2_bootinfo_init(void);

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

#define X2_QSPI_RE_FRQ          (1 << 3)
#define X2_QSPI_RE_FRQ_SHIFT    (4)
#define X2_QSPI_RE_MODE         (1 << 2)
#define X2_QSPI_RE_MODE_MASK    (0x3)

void spl_nor_init(unsigned int cfg);
int spi_flash_read(u32 offset, size_t len, void *data);

#endif

#endif
