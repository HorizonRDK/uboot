/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#include <asm/io.h>
#include <asm/arch/hb_dev.h>
#include <linux/mtd/nand.h>
#include <asm/arch/hb_pinmux.h>
#include "hb_spi_spl.h"
#include "hb_nand_spl.h"
#include "hb_info.h"

#ifdef HB_NAND_BOOT
static const struct spinand_manufacturer spinand_manufacturers[] = {
	{0xC8, "GigaDev"},
	{0xEF, "Winbond"},
	{0x2C, "Micro"},
	{0xC2, "Macronix"},
};

static struct spinand_chip g_nand_chip;
static uint8_t oob_buf[128] = {[0 ... 127] = 0xff};
int spi_mem_exec_op(struct spi_slave *slave, const struct spi_mem_op *op)
{
	unsigned int pos = 0;
	const uint8_t *tx_buf = NULL;
	uint8_t *rx_buf = NULL;
	uint8_t op_buf[8];
	int op_len;
	uint32_t flag;
	int ret;
	int i;
#if 0
	/* U-Boot does not support parallel SPI data lanes */
	if ((op->cmd.buswidth != 1) ||
			(op->addr.nbytes && op->addr.buswidth != 1) ||
			(op->dummy.nbytes && op->dummy.buswidth != 1) ||
			(op->data.nbytes && op->data.buswidth != 1)) {
		printf("Dual/Quad raw SPI transfers not supported\n");
		return -1;
	}
#endif
	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN)
			rx_buf = op->data.buf.in;
		else
			tx_buf = op->data.buf.out;
	}

	op_len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;

	spl_spi_claim_bus(slave);
	op_buf[pos++] = op->cmd.opcode;

	if (op->addr.nbytes) {
		for (i = 0; i < op->addr.nbytes; i++)
			op_buf[pos + i] = op->addr.val >>
					(8 * (op->addr.nbytes - i - 1));

		pos += op->addr.nbytes;
	}

	if (op->dummy.nbytes)
		memset(op_buf + pos, 0x0, op->dummy.nbytes);

	/* 1st transfer: opcode + address + dummy cycles */
	flag = SPI_XFER_BEGIN;
	/* Make sure to set END bit if no tx or rx data messages follow */
	if (!tx_buf && !rx_buf)
		flag |= SPI_XFER_END;

	ret = spl_spi_xfer(slave, op_len * 8, op_buf, NULL, flag);
	if (ret)
		return ret;

	/* 2nd transfer: rx or tx data path */
	if (tx_buf || rx_buf) {
		ret = spl_spi_xfer(slave, op->data.nbytes * 8, tx_buf,
						 rx_buf, SPI_XFER_END);
		if (ret)
			return ret;
	}

	spl_spi_release_bus(slave);
#if 0
	for (i = 0; i < pos; i++)
		debug("%02x ", op_buf[i]);
	debug("| [%dB %s] ",
				tx_buf || rx_buf ? op->data.nbytes : 0,
				tx_buf || rx_buf ? (tx_buf ? "out" : "in") : "-");
	for (i = 0; i < op->data.nbytes; i++)
		debug("%02x ", tx_buf ? tx_buf[i] : rx_buf[i]);
	debug("[ret %d]\n", ret);
#endif
	return 0;
}

static int spinand_write_reg_op(struct spinand_chip *spinand, uint8_t reg,
	uint8_t val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg, spinand->scratchbuf);

	*spinand->scratchbuf = val;
	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_read_reg_op(struct spinand_chip *spinand, uint8_t reg,
						 uint8_t * val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg, spinand->scratchbuf);
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	*val = *spinand->scratchbuf;
	return 0;
}

static int spinand_lock_block(struct spinand_chip *spinand, uint8_t lock)
{
	return spinand_write_reg_op(spinand, REG_BLOCK_LOCK, lock);
}

static int spinand_read_status(struct spinand_chip *spinand, uint8_t * status)
{
	return spinand_read_reg_op(spinand, REG_STATUS, status);
}

static int spinand_load_page_op(struct spinand_chip *spinand,
				const struct nand_page_io_req *req)
{
	unsigned int row = (req->pos.eraseblock << spinand->memorg.eraseblock_addr_shift) |
		req->pos.page;

	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_wait(struct spinand_chip *spinand, uint8_t * s)
{
	unsigned long start, stop;
	uint8_t status;
	int ret;

	start = 0;
	stop = 400;

	do {
		ret = spinand_read_status(spinand, &status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;

		start++;
	} while (start < stop);

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(spinand, &status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -ETIMEDOUT : 0;
}

static inline unsigned int spinand_pos_to_row(struct spinand_chip *nand,
	const struct nand_pos *pos)
{
	return (pos->eraseblock << nand->memorg.eraseblock_addr_shift) | pos->page;
}

static void spinand_cache_op_adjust_colum(struct spinand_chip *spinand,
	const struct nand_page_io_req *req,
	uint16_t *column)
{
	unsigned int shift;

	if (spinand->memorg.planes_per_lun < 2) {
		return;
	}

	/* The plane number is passed in MSB just above the column address */
	shift = fls(spinand->memorg.pagesize);
	*column |= (req->pos.eraseblock % spinand->memorg.planes_per_lun) << shift;
}


static int spinand_read_from_cache_op(struct spinand_chip *spinand,
	const struct nand_page_io_req *req)
{
	struct spi_mem_op op = SPINAND_PAGE_READ_FROM_CACHE_OP(0, 0, 1, NULL, 0);
	unsigned int nbytes = 0;
	void *buf = NULL;
	uint16_t column = 0;
	int ret;

	if (req->datalen == 0) {
		return -1;
	}
	buf = req->databuf.in;
	nbytes = req->datalen;

	spinand_cache_op_adjust_colum(spinand, req, &column);
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max RX data size. In this
	 * case, just repeat the READ_CACHE operation after updating the
	 * column.
	 */
	while (nbytes) {
		op.data.buf.in = buf;
		op.data.nbytes = nbytes;

		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	buf = req->oobbuf.in;
	nbytes = req->ooblen;
	op.addr.val = 2048;
        while (nbytes) {
		op.data.buf.in = buf;
		op.data.nbytes = nbytes;

		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	return 0;
}

static int spinand_read_page(struct spinand_chip *spinand,
	uint32_t offset, void *data)
{
	struct nand_page_io_req req;
	uint8_t status;
	int ret;

	memset(&req, 0x0, sizeof(req));

	req.pos.eraseblock =
			offset / (spinand->memorg.pages_per_eraseblock *
					spinand->memorg.pagesize);
	req.pos.page = (offset %
		(spinand->memorg.pages_per_eraseblock * spinand->memorg.pagesize)) /
		spinand->memorg.pagesize;
	req.databuf.in = data;
	req.datalen = spinand->memorg.pagesize;
	req.dataoffs = 0;
	req.ooblen = spinand->memorg.oobsize;
	req.oobbuf.in = &oob_buf;
	req.ooboffs = 0;
	ret = spinand_load_page_op(spinand, &req);
	if (ret) {
		return ret;
	}

	ret = spinand_wait(spinand, &status);
	if (ret < 0) {
		return ret;
	}

	ret = spinand_read_from_cache_op(spinand, &req);
	if (ret) {
		return ret;
	}
	if (oob_buf[0] == 0x00) {
		return 0xbb;
	}

	return 0;
}

size_t spinand_mtd_read(int offset, uint64_t data, size_t len)
{
	struct spinand_chip *spinand = &g_nand_chip;
	uint8_t *pbuf = (uint8_t *)data;
	uint32_t addr = offset;
	int ret = 0;
	int count = 0;

	while ((int64_t)len > 0) {
		ret = spinand_read_page(spinand, addr, pbuf);
		if (ret == 0xbb) {
			printf("bad block encounter, skipping to next block\n");
			addr += spinand->memorg.pagesize * spinand->memorg.pages_per_eraseblock;
		} else if (ret < 0) {
			return ret;
		} else {
			addr += spinand->memorg.pagesize;
			pbuf += spinand->memorg.pagesize;
			len -= spinand->memorg.pagesize;
			count += spinand->memorg.pagesize;
		}
	}

	return count;
}

#ifdef CONFIG_X2_PM
static int spinand_write_enable_op(struct spinand_chip *spinand)
{
	struct spi_mem_op op = SPINAND_WR_EN_DIS_OP(true);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_write_to_cache_op(struct spinand_chip *spinand,
	const struct nand_page_io_req *req)
{
	struct spi_mem_op op = SPINAND_PROG_LOAD(true, 0, NULL, 0);
	unsigned int nbytes = 0;
	const void *buf = NULL;
	u16 column = 0;
	int ret;

	if (req->datalen == 0)
		return 0;

	buf = req->databuf.out;
	nbytes = req->datalen;

	spinand_cache_op_adjust_colum(spinand, req, &column);
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max TX data size. In this
	 * case, split the operation into one LOAD CACHE and one or more
	 * LOAD RANDOM CACHE.
	 */
	while (nbytes) {
		op.data.buf.out = buf;
		op.data.nbytes = nbytes;

		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	return 0;
}

static int spinand_program_op(struct spinand_chip *spinand,
	const struct nand_page_io_req *req)
{
	unsigned int row = spinand_pos_to_row(spinand, &req->pos);
	struct spi_mem_op op = SPINAND_PROG_EXEC_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_write_page(struct spinand_chip *spinand,
	uint32_t offset, void *data)
{
	struct nand_page_io_req req;
	uint8_t status;
	int ret;

	memset(&req, 0x0, sizeof(req));

	req.pos.eraseblock =
		offset / (spinand->memorg.pages_per_eraseblock *
			spinand->memorg.pagesize);
	req.pos.page = (offset %
		(spinand->memorg.pages_per_eraseblock * spinand->memorg.pagesize)) /
		spinand->memorg.pagesize;
	req.databuf.out = data;
	req.datalen = spinand->memorg.pagesize;
	req.dataoffs = 0;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_write_to_cache_op(spinand, &req);
	if (ret)
		return ret;

	ret = spinand_program_op(spinand, &req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_PROG_FAILED))
		ret = -EIO;

	return ret;
}

uint32_t spinand_mtd_write(uint32_t offset, uintptr_t data, size_t len)
{
	struct spinand_chip *spinand = &g_nand_chip;
	uint8_t *pbuf = (uint8_t *)data;
	uint32_t addr = offset;
	int ret = 0;
	int count = 0;

	while (len > 0) {
		ret = spinand_write_page(spinand, addr, pbuf);
		if (ret < 0) {
			return ret;
		}

		addr += spinand->memorg.pagesize;
		pbuf += spinand->memorg.pagesize;
		len -= spinand->memorg.pagesize;
		count += spinand->memorg.pagesize;
	}

	return count;
}

static int spinand_erase_op(struct spinand_chip *spinand,
	const struct nand_pos *pos)
{
	unsigned int row = spinand_pos_to_row(spinand, pos);
	struct spi_mem_op op = SPINAND_BLK_ERASE_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

int spinand_mtd_erase(uint32_t offset, size_t len)
{
	struct spinand_chip *spinand = &g_nand_chip;
	struct nand_pos pos;
	uint8_t status;
	int ret;

	if (len == 0)
		return 0;

	memset(&pos, 0, sizeof(pos));

	pos.eraseblock = offset / (spinand->memorg.pages_per_eraseblock *
		spinand->memorg.pagesize);
	pos.page = (offset %
		(spinand->memorg.pages_per_eraseblock * spinand->memorg.pagesize)) /
		spinand->memorg.pagesize;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_erase_op(spinand, &pos);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_ERASE_FAILED))
		ret = -EIO;

	return ret;
}
#endif /* CONFIG_X2_PM */

static int spinand_read_id_op(struct spinand_chip *spinand, uint8_t * buf)
{
	struct spi_mem_op op = SPINAND_READID_OP(0, spinand->scratchbuf,
						 SPINAND_MAX_ID_LEN);
	int ret;

	if (!spinand->flags) {
		op.dummy.nbytes = 1;
	}

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (!ret)
		memcpy(buf, spinand->scratchbuf, SPINAND_MAX_ID_LEN);

	return ret;
}

static int spinand_reset_op(struct spinand_chip *spinand)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	return spinand_wait(spinand, NULL);
}

static int spinand_manufacturer_detect(struct spinand_chip *spinand)
{
	unsigned int i;
	uint8_t *id = spinand->id.data;

	for (i = 0; i < ARRAY_SIZE(spinand_manufacturers); i++) {
		if (id[0] == spinand_manufacturers[i].id ||
			id[1] == spinand_manufacturers[i].id) {
			spinand->manufacturer = &spinand_manufacturers[i];
			return 0;
		}
	}

	spinand->manufacturer = NULL;

	return -1;
}

static int spinand_detect(struct spinand_chip *spinand, unsigned int reset)
{
	int ret;

	if (reset > 0) {
		ret = spinand_reset_op(spinand);
		if (ret)
			return ret;
	}

	ret = spinand_read_id_op(spinand, spinand->id.data);
	if (ret)
		return ret;

	spinand->id.len = SPINAND_MAX_ID_LEN;

	ret = spinand_manufacturer_detect(spinand);
	if (ret) {
		printf("Unknown raw ID %x\n", spinand->id.data[0]);
		return ret;
	}

	return 0;
}

static int spinand_init(struct spinand_chip *dev, uint32_t page_flag,
			uint32_t reset)
{
	int ret = 0;
	struct nand_mem_org *pmemorg;

	ret = spinand_detect(dev, reset);
	dev->databuf = NULL;

	pmemorg = &dev->memorg;
	pmemorg->bits_per_cell = 1;
	pmemorg->pagesize = (page_flag > 0 ? 4096 : 2048);
	pmemorg->oobsize = (page_flag > 0 ? 128 : 64);
	pmemorg->pages_per_eraseblock = 64;
	pmemorg->eraseblock_addr_shift = fls(pmemorg->pages_per_eraseblock - 1);

	pmemorg->planes_per_lun = hb_pin_get_nand_lun();
	pmemorg->luns_per_target = 1;
	pmemorg->ntargets = 1;

	printf("%s SPI NAND was found.\n",
				 dev->manufacturer ? dev->manufacturer->name : "Unknown");
	printf("Block size: %u KiB\n", (dev->memorg.pages_per_eraseblock *
		dev->memorg.pagesize) >> 10);
        printf("Page size: %u B\n\n", dev->memorg.pagesize);

        /* After power up, all blocks are locked, so unlock them here. */
	ret = spinand_lock_block(dev, BL_ALL_UNLOCKED);
	if (ret) {
		return ret;
	}

	return 0;
}

int spinand_probe(uint32_t spi_num, uint32_t page_flag, uint32_t id_dummy,
			uint32_t reset, uint32_t mclk, uint32_t sclk)
{
	struct spi_slave *pslave;
	struct spinand_chip *pflash = &g_nand_chip;
	int ret = 0;

#ifdef CONFIG_QSPI_NAND_DUAL
	pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_DUAL);
#elif defined(CONFIG_QSPI_NAND_QUAD)
	pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_QUAD);
#else
	pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_SLOW);
#endif /* CONFIG_QSPI_DUAL */

	pflash->slave = pslave;
	pflash->flags = id_dummy;

	ret = spinand_init(pflash, page_flag, reset);
	if (ret) {
		return ret;
	}

	return 0;
}

static uint32_t nand_read_blks(uint32_t lba, uint64_t buf, size_t size)
{
	return spinand_mtd_read(lba, buf, size);
}

static void nand_pre_load(struct hb_info_hdr *pinfo,
				int tr_num, int tr_type,
				unsigned int *pload_addr, unsigned int *pload_size)
{
	if (tr_num == 0) {
		if (tr_type == 0) {
			*pload_addr = pinfo->ddt1_addr[0];
			*pload_size = pinfo->ddt1_imem_size;
		} else {
			*pload_addr = pinfo->ddt1_addr[0] + 0x8000;
			*pload_size = pinfo->ddt1_dmem_size;
		}
	} else {
		if (tr_type == 0) {
			*pload_addr = pinfo->ddt2_addr[0];
			*pload_size = pinfo->ddt2_imem_size;
		} else {
			*pload_addr = pinfo->ddt2_addr[0] + 0x8000;
			*pload_size = pinfo->ddt2_dmem_size;
		}
	}

	return;
}

static void nand_load_image(struct hb_info_hdr *pinfo)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;
	__maybe_unused unsigned int read_bytes;

	src_addr = pinfo->other_img[0].img_addr;
	src_len = pinfo->other_img[0].img_size;
	dest_addr = pinfo->other_laddr;

	read_bytes = nand_read_blks((int)src_addr, dest_addr, src_len);

	return;
}

void hb_bootinfo_init(void)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;

	src_addr = 0x0;
	src_len = 0x200;
	dest_addr = HB_BOOTINFO_ADDR;

	nand_read_blks((int)src_addr, dest_addr, src_len);
}

void spl_nand_init(void)
{
	int dm = hb_pin_get_nand_dummy();
	int dev_mode = hb_pin_get_dev_mode();

	debug("dummy=%d, dev_mode=%d\n", dm, dev_mode);

	spinand_probe(0, dev_mode, dm, 0, HB_NAND_MCLK, HB_NAND_SCLK);
	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = nand_pre_load;
	g_dev_ops.read = nand_read_blks;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = nand_load_image;

#ifdef CONFIG_X2_PM
	g_dev_ops.write = spinand_mtd_write;
	g_dev_ops.erase = spinand_mtd_erase;
#endif /* CONFIG_X2_PM */

	return;
}

#endif /* HB_NAND_BOOT */
