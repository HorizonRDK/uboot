#ifndef __HOBOT_NOR_FLASH_H__
#define __HOBOT_NOR_FLASH_H__

#ifdef CONFIG_X2_NOR_BOOT

/* #define CONFIG_QSPI_DUAL */
#define CONFIG_QSPI_QUAD
#define SPI_FLASH_MAX_ID_LEN	8

void x2_bootinfo_init(void);

void spl_nor_init(void);
int spi_flash_read(u32 offset, size_t len, void *data);

#endif

#endif
