#include <asm/io.h>
#include <asm/arch/x2_dev.h>
#include <linux/mtd/spinand.h>
#include <linux/mtd/nand.h>
#include <spi-mem.h>
#include <spi.h>
#include "x2_spi_spl.h"
#include "x2_nand_spl.h"
#include "x2_info.h"
#include "x2_info_spl.h"

#ifdef CONFIG_X2_NAND_BOOT
static const struct spinand_manufacturer spinand_manufacturers[] = {
	{0xC8, "GigaDev"},
	{0xEF, "Winbond"},
	{0x2C, "Micro"},
	{0xC2, "Macronix"},
};

static struct spinand_device g_nand_dev;

uint32_t g_shift = 0;

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

	/* U-Boot does not support parallel SPI data lanes */
	if ((op->cmd.buswidth != 1) ||
	    (op->addr.nbytes && op->addr.buswidth != 1) ||
	    (op->dummy.nbytes && op->dummy.buswidth != 1) ||
	    (op->data.nbytes && op->data.buswidth != 1)) {
		printf("Dual/Quad raw SPI transfers not supported\n");
		return -1;
	}

	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN)
			rx_buf = op->data.buf.in;
		else
			tx_buf = op->data.buf.out;
	}

	op_len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;

	spi_claim_bus(slave);
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

	ret = spi_xfer(slave, op_len * 8, op_buf, NULL, flag);
	if (ret)
		return ret;

	/* 2nd transfer: rx or tx data path */
	if (tx_buf || rx_buf) {
		ret = spi_xfer(slave, op->data.nbytes * 8, tx_buf,
			       rx_buf, SPI_XFER_END);
		if (ret)
			return ret;
	}

	spi_release_bus(slave);

	return 0;
}

static int spinand_write_reg_op(struct spinand_device *spinand, uint8_t reg,
				uint8_t val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg, spinand->scratchbuf);

	*spinand->scratchbuf = val;
	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_read_reg_op(struct spinand_device *spinand, uint8_t reg,
			       uint8_t * val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg,
						      spinand->scratchbuf);
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	*val = *spinand->scratchbuf;
	return 0;
}

static int spinand_lock_block(struct spinand_device *spinand, uint8_t lock)
{
	return spinand_write_reg_op(spinand, REG_BLOCK_LOCK, lock);
}

static int spinand_read_status(struct spinand_device *spinand, uint8_t * status)
{
	return spinand_read_reg_op(spinand, REG_STATUS, status);
}

static int spinand_load_page_op(struct spinand_device *spinand,
				const struct nand_page_io_req *req)
{
	unsigned int row = (req->pos.eraseblock << g_shift) | req->pos.page;
	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_wait(struct spinand_device *spinand, uint8_t * s)
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

		udelay(2);
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

static int spinand_read_from_cache_op(struct spinand_device *spinand,
				      const struct nand_page_io_req *req)
{
	struct spi_mem_op op =
	    SPINAND_PAGE_READ_FROM_CACHE_OP(0, 0, 1, NULL, 0);
	struct nand_page_io_req adjreq = *req;
	unsigned int nbytes = 0;
	void *buf = NULL;
	uint16_t column = 0;
	int ret;

	if (req->datalen) {
		adjreq.datalen = spinand->base.memorg.pagesize;
		adjreq.dataoffs = 0;
		buf = req->databuf.in;
		nbytes = adjreq.datalen;
	}

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

	return 0;
}

static int spinand_read_page(struct spinand_device *spinand,
			     uint32_t offset, void *data)
{
	struct nand_page_io_req req;
	uint8_t status;
	int ret;

	memset(&req, 0x0, sizeof(req));

	req.pos.eraseblock =
	    offset / (spinand->base.memorg.pages_per_eraseblock *
		      spinand->base.memorg.pagesize);
	req.pos.page =
	    (offset %
	     (spinand->base.memorg.pages_per_eraseblock *
	      spinand->base.memorg.pagesize)) / spinand->base.memorg.pagesize;
	req.databuf.in = data;
	req.datalen = spinand->base.memorg.pagesize;
	req.dataoffs = 0;
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

	return 0;
}

size_t spinand_mtd_read(int offset, uintptr_t data, size_t len)
{
	struct spinand_device *spinand = &g_nand_dev;
	uint8_t *pbuf = (uint8_t *) data;
	uint32_t addr = offset * spinand->base.memorg.pagesize;
	int ret = 0;
	int count = 0;

	while (len > 0) {
		ret = spinand_read_page(spinand, addr, pbuf);
		if (ret < 0) {
			return ret;
		}

		addr += spinand->base.memorg.pagesize;
		pbuf += spinand->base.memorg.pagesize;
		len -= spinand->base.memorg.pagesize;
		count += spinand->base.memorg.pagesize;
	}

	return count;
}

static int spinand_read_id_op(struct spinand_device *spinand, uint8_t * buf)
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

static int spinand_reset_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	return spinand_wait(spinand, NULL);
}

static int spinand_manufacturer_detect(struct spinand_device *spinand)
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

static int spinand_detect(struct spinand_device *spinand, unsigned int reset)
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

static int spinand_init(struct spinand_device *dev, uint32_t page_flag,
			uint32_t reset)
{
	int ret = 0;
	struct nand_memory_organization *pmemorg;

	ret = spinand_detect(dev, reset);
	dev->databuf = NULL;
	pmemorg = &(dev->base.memorg);
	pmemorg->bits_per_cell = 1;
	pmemorg->pagesize = (page_flag > 0 ? 4096 : 2048);
	pmemorg->oobsize = (page_flag > 0 ? 256 : 128);
	pmemorg->pages_per_eraseblock = 64;
	pmemorg->planes_per_lun = x2_get_spinand_lun();
	g_shift = fls(pmemorg->pages_per_eraseblock - 1);

	printf("%s SPI NAND was found.\n",
	       dev->manufacturer ? dev->manufacturer->name : "Unknown");
	printf("Block size: %u KiB, page size: %u\n",
	       (dev->base.memorg.pages_per_eraseblock *
		dev->base.memorg.pagesize) >> 10, dev->base.memorg.pagesize);

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
	struct spinand_device *pflash = &g_nand_dev;
	int ret = 0;

#ifdef CONFIG_QSPI_NAND_DUAL
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_DUAL);
#elif defined(CONFIG_QSPI_NAND_QUAD)
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_QUAD);
#else
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_SLOW);
#endif /* CONFIG_QSPI_DUAL */

	pflash->slave = pslave;
	pflash->flags = id_dummy;

	ret = spinand_init(pflash, page_flag, reset);
	if (ret) {
		return ret;
	}

	return 0;
}

static unsigned int nand_read_blks(int lba, uint64_t buf, size_t size)
{
	return spinand_mtd_read(lba, (uintptr_t) buf, size);
}

static void nand_pre_load(struct x2_info_hdr *pinfo,
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

static void nand_load_image(struct x2_info_hdr *pinfo)
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

void spl_nand_init(void)
{
	int dm = x2_get_spinand_dummy();
	int dev_mode = x2_get_dev_mode();
	int rest = x2_get_reset_sf();

	printf("%s dummy=%d, dev_mode=%d, reset=%d\n", __func__, dm, dev_mode,
	       rest);
	spinand_probe(0, dev_mode, dm, rest, X2_NAND_MCLK, X2_NAND_SCLK);
	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = nand_pre_load;
	g_dev_ops.read = nand_read_blks;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = nand_load_image;
	return;
}
#endif
