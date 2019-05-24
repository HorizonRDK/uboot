#ifndef __HOBOT_SPI_H__
#define __HOBOT_SPI_H__

#if defined(CONFIG_X2_NOR_BOOT) || defined(CONFIG_X2_NAND_BOOT)

#include <spi.h>

#define QSPI_DEV_CS0		        0x1
#define X2_QSPI_MCLK		(500000000)
#define X2_QSPI_SCLK		(50000000)

int spl_spi_claim_bus(struct spi_slave *slave);

int spl_spi_release_bus(struct spi_slave *slave);

struct spi_slave *spl_spi_setup_slave(unsigned int bus,
	unsigned int cs, unsigned int max_hz, unsigned int mode);

int spl_spi_xfer(struct spi_slave *slave, unsigned int bitlen,
	const void *dout, void *din, unsigned long flags);

#endif
#endif
