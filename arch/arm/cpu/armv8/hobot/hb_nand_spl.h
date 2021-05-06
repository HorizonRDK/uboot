/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#ifndef __HB_NAND_SPL_H__
#define __HB_NAND_SPL_H__

#ifdef HB_NAND_BOOT

#include <spi.h>
#include <linux/mtd/spinand.h>
#include <spi-mem.h>

#define HB_NAND_MCLK        (500000000)
#define HB_NAND_SCLK        (12500000)
//#define CONFIG_QSPI_NAND_QUAD

struct nand_mem_org {
	unsigned int bits_per_cell;
	unsigned int pagesize;
	unsigned int oobsize;
	unsigned int pages_per_eraseblock;
	unsigned int eraseblock_addr_shift;
	unsigned int eraseblocks_per_lun;
	unsigned int planes_per_lun;
	unsigned int luns_per_target;
	unsigned int ntargets;
};

/**
 * struct spinand_chip - SPI NAND chip instance
 * @base: NAND device instance
 * @slave: pointer to the SPI slave object
 * @lock: lock used to serialize accesses to the NAND
 * @id: NAND ID as returned by READ_ID
 * @flags: NAND flags
 * @op_templates: various SPI mem op templates
 * @op_templates.read_cache: read cache op template
 * @op_templates.write_cache: write cache op template
 * @op_templates.update_cache: update cache op template
 * @select_target: select a specific target/die. Usually called before sending
 *		   a command addressing a page or an eraseblock embedded in
 *		   this die. Only required if your chip exposes several dies
 * @cur_target: currently selected target/die
 * @eccinfo: on-die ECC information
 * @cfg_cache: config register cache. One entry per die
 * @databuf: bounce buffer for data
 * @oobbuf: bounce buffer for OOB data
 * @scratchbuf: buffer used for everything but page accesses. This is needed
 *		because the spi-mem interface explicitly requests that buffers
 *		passed in spi_mem_op be DMA-able, so we can't based the bufs on
 *		the stack
 * @manufacturer: SPI NAND manufacturer information
 * @priv: manufacturer private data
 */
struct spinand_chip {
	struct spi_slave *slave;
	struct spinand_id id;
	uint32_t flags;
	uint32_t is_dummy;
	uint32_t plane_num;

	struct {
		const struct spi_mem_op *read_cache;
		const struct spi_mem_op *write_cache;
		const struct spi_mem_op *update_cache;
	} op_templates;

	struct nand_mem_org memorg;

	uint8_t *cfg_cache;
	uint8_t *databuf;
	uint8_t *oobbuf;
	uint8_t scratchbuf[SPINAND_MAX_ID_LEN];
	const struct spinand_manufacturer *manufacturer;
	void *priv;
};

void spl_nand_init(void);
void hb_bootinfo_init(void);

#endif /* HB_NAND_BOOT */
#endif /* __HB_NAND_SPL_H__ */
