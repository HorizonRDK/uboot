#ifndef __HOBOT_SPI_H__
#define __HOBOT_SPI_H__

#if defined(HB_NOR_BOOT) || defined(HB_NAND_BOOT)

#include <spi.h>

#define QSPI_DEV_CS0		        0x1
#define HB_QSPI_MCLK		(500000000)
#define HB_QSPI_SCLK		(50000000)

#define HB_QSPI_SLOW_MODE       (0x0)
#define HB_QSPI_DUAL_MODE       (0x1)
#define HB_QSPI_QUAD_MODE       (0x2)

unsigned int spl_spi_set_speed(struct spi_slave *slave, uint hz);
int spl_spi_claim_bus(struct spi_slave *slave);

int spl_spi_release_bus(struct spi_slave *slave);

struct spi_slave *spl_spi_setup_slave(unsigned int bus,
	unsigned int cs, unsigned int max_hz, unsigned int mode);

int spl_spi_xfer(struct spi_slave *slave, unsigned int bitlen,
	const void *dout, void *din, unsigned long flags);

#endif
#endif
