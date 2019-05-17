#include <asm/io.h>
#include <asm/arch/x2_dev.h>
#include <spi.h>
#include "x2_nor_spl.h"
#include "x2_spi_spl.h"
#include "x2_info.h"

#ifdef CONFIG_X2_NOR_BOOT

#define SECT_4K			BIT(0)	/* CMD_ERASE_4K works uniformly */
#define E_FSR			BIT(1)	/* use flag status register for */
#define SST_WR			BIT(2)	/* use SST byte/word programming */
#define WR_QPP			BIT(3)	/* use Quad Page Program */
#define RD_QUAD			BIT(4)	/* use Quad Read */
#define RD_DUAL			BIT(5)	/* use Dual Read */
#define RD_QUADIO		BIT(6)	/* use Quad IO Read */
#define RD_DUALIO		BIT(7)	/* use Dual IO Read */
#define RD_FULL			(RD_QUAD | RD_DUAL | RD_QUADIO | RD_DUALIO)

/* Flash timeout values */
#define SPI_FLASH_PROG_TIMEOUT		(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_PAGE_ERASE_TIMEOUT	(5 * CONFIG_SYS_HZ)

/* Common status */
#define STATUS_WIP			BIT(0)
#define STATUS_QEB_WINSPAN		BIT(1)
#define STATUS_QEB_MXIC			BIT(6)
#define STATUS_PEC			BIT(7)

/* CFI Manufacture ID's */
#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
#define SPI_FLASH_CFI_MFR_SPANSION	0x01
#define SPI_FLASH_CFI_MFR_WINBOND	0xef
#endif

#ifdef CONFIG_SPI_FLASH_STMICRO
#define SPI_FLASH_CFI_MFR_STMICRO	0x20
#endif

#ifdef CONFIG_SPI_FLASH_MACRONIX
#define SPI_FLASH_CFI_MFR_MACRONIX	0xc2
#endif

#define CMD_OP_EN4B		0xb7	/* Enter 4-byte mode */
#define CMD_OP_EX4B		0xe9	/* Exit 4-byte mode */

#define JEDEC_MFR(info)		((info)->id[0])

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = (!(_jedec_id) ? 0 : (3 + ((_ext_id) ? 2 : 0))),	\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),

/* Dual SPI flash memories - see SPI_COMM_DUAL_... */
enum spi_dual_flash {
	SF_SINGLE_FLASH = 0,
	SF_DUAL_STACKED_FLASH = BIT(0),
	SF_DUAL_PARALLEL_FLASH = BIT(1),
};

enum spi_nor_option_flags {
	SNOR_F_SST_WR = BIT(0),
	SNOR_F_USE_FSR = BIT(1),
	SNOR_F_USE_UPAGE = BIT(3),
};

struct spi_flash_info {
	/* Device name ([MANUFLETTER][DEVTYPE][DENSITY][EXTRAINFO]) */
	const char *name;
	/*
	 * This array stores the ID bytes.
	 * The first three bytes are the JEDIC ID.
	 * JEDEC ID zero means "no ID" (mostly older chips).
	 */
	u8 id[SPI_FLASH_MAX_ID_LEN];
	u8 id_len;
	/*
	 * The size listed here is what works with SPINOR_OP_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	u32 sector_size;
	u32 n_sectors;
	u16 page_size;
	u16 flags;
};

struct spi_flash {
	struct spi_slave *spi;
	const char *name;
	uint8_t dual_flash;
	uint8_t shift;
	uint16_t flags;
	uint8_t addr_width;
	uint32_t size;
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t erase_size;
	uint8_t erase_cmd;
	uint8_t read_cmd;
	uint8_t write_cmd;
	uint8_t dummy_byte;
};

static struct spi_flash g_spi_flash;

const struct spi_flash_info spi_flash_ids[] = {
	{"gd25q256c/d", INFO(0xC84019, 0x0, 64 * 1024, 8192, SECT_4K | RD_DUAL)},
	{"w25q256fv", INFO(0xEF4019, 0x0, 64 * 1024, 8192, SECT_4K | RD_DUAL)},
	{"n25q256a", INFO(0x20BA19, 0x0, 64 * 1024, 8192, SECT_4K)},
	{"gd25lq256d",
	 INFO(0xc86019, 0x0, 64 * 1024, 512, RD_FULL | WR_QPP | SECT_4K)},
	{"gd25lq128d",
	 INFO(0xc86018, 0x0, 64 * 1024, 256, RD_FULL | WR_QPP | SECT_4K) },
	{},			/* Empty entry to terminate the list */
};

const struct spi_flash_info spi_flash_def =
{ "unknown", INFO(0x0, 0x0, 64 * 1024, 512, RD_FULL | WR_QPP | SECT_4K) };

static int spi_flash_read_write(struct spi_slave *spi,
				const uint8_t * cmd, size_t cmd_len,
				const uint8_t * data_out, uint8_t * data_in,
				size_t data_len)
{
	unsigned long flags = SPI_XFER_BEGIN;
	int ret;

	if (data_len == 0)
		flags |= SPI_XFER_END;

	ret = spi_xfer(spi, cmd_len * 8, cmd, NULL, flags | SPI_XFER_CMD);
	if (ret) {
		printf("SF: Failed to send command (%ld bytes): %d\n", cmd_len,
		       ret);
	} else if (data_len != 0) {
		ret =
			spi_xfer(spi, data_len * 8, data_out, data_in,
				 SPI_XFER_END);
		if (ret)
			printf("SF: Failed to transfer %ld bytes of data: %d\n",
			       data_len, ret);
	}

	return ret;
}

static int spi_flash_cmd_read(struct spi_slave *spi, const uint8_t * cmd,
			      size_t cmd_len, void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, NULL, data, data_len);
}

static int spi_flash_cmd(struct spi_slave *spi, uint8_t cmd, void *response,
			 size_t len)
{
	return spi_flash_cmd_read(spi, &cmd, 1, response, len);
}

static int spi_flash_reset(struct spi_flash *flash)
{
	int ret;
	struct spi_slave *spi = (struct spi_slave *)flash->spi;

	spi_claim_bus(spi);
	ret = spi_flash_cmd(flash->spi, CMD_RESET_EN, NULL, 0);
	spi_release_bus(spi);

	udelay(2);

	spi_claim_bus(spi);
	ret = spi_flash_cmd(flash->spi, CMD_RESET, NULL, 0);
	spi_release_bus(spi);

	udelay(100);

	return ret;
}

static const struct spi_flash_info *spi_flash_read_id(struct spi_flash *flash)
{
	int tmp;
	uint8_t id[SPI_FLASH_MAX_ID_LEN];
	const struct spi_flash_info *info;
	struct spi_slave *spi = (struct spi_slave *)flash->spi;

	memset(id, 0, sizeof(id));

	spi_claim_bus(spi);
	tmp = spi_flash_cmd(flash->spi, CMD_READ_ID, id, SPI_FLASH_MAX_ID_LEN);
	spi_release_bus(spi);

	if (tmp < 0) {
		printf("SF: error %d reading JEDEC ID\n", tmp);
		return NULL;
	}

	info = spi_flash_ids;
	for (; info->name != NULL; info++) {
		if (info->id_len) {
			if (!memcmp(info->id, id, info->id_len))
				return info;
		}
	}

	printf("SPL SF: JEDEC id: %02x, %02x, %02x\nUsing default configure info\n",
	       id[0], id[1], id[2]);
	return &spi_flash_def;
}

static int spi_flash_read_common(struct spi_flash *flash, const uint8_t * cmd,
				 size_t cmd_len, void *data, size_t data_len)
{
	struct spi_slave *spi = flash->spi;
	int ret;

	spi_claim_bus(spi);
	ret = spi_flash_cmd_read(spi, cmd, cmd_len, data, data_len);
	spi_release_bus(spi);
	if (ret < 0) {
		printf("SF: read cmd failed\n");
		return ret;
	}

	return data_len;
}

#if defined(CONFIG_SPI_FLASH_MACRONIX) || defined(CONFIG_SPI_FLASH_SPANSION) || \
    defined(CONFIG_SPI_FLASH_WINBOND)
/* Enable writing on the SPI flash */
static int spi_flash_sr_ready(struct spi_flash *flash);
static int spi_flash_cmd_write_enable(struct spi_flash *flash)
{
	return spi_flash_cmd(flash->spi, CMD_WRITE_ENABLE, NULL, 0);
}

static int read_fsr(struct spi_flash *flash, u8 * fsr)
{
	int ret;
	const u8 cmd = CMD_FLAG_STATUS;

	ret = spi_flash_read_common(flash, &cmd, 1, fsr, 1);
	if (ret < 0) {
		debug("SF: fail to read flag status register\n");
		return ret;
	}

	return 0;
}

static int spi_flash_fsr_ready(struct spi_flash *flash)
{
	u8 fsr;
	int ret;

	ret = read_fsr(flash, &fsr);
	if (ret < 0)
		return ret;

	return fsr & STATUS_PEC;
}

static int spi_flash_ready(struct spi_flash *flash)
{
	int sr, fsr;

	sr = spi_flash_sr_ready(flash);
	if (sr < 0)
		return sr;

	fsr = 1;
	if (flash->flags & SNOR_F_USE_FSR) {
		fsr = spi_flash_fsr_ready(flash);
		if (fsr < 0)
			return fsr;
	}

	return sr && fsr;
}

static int spi_flash_wait_till_ready(struct spi_flash *flash,
				     unsigned long timeout)
{
	unsigned long timebase;
	int ret;

	timebase = get_timer(0);

	while (get_timer(timebase) < timeout) {
		ret = spi_flash_ready(flash);
		if (ret < 0)
			return ret;
		if (ret)
			return 0;
	}

	printf("SF: Timeout!\n");

	return -ETIMEDOUT;
}

static int spi_flash_cmd_write(struct spi_slave *spi, const u8 * cmd,
			       size_t cmd_len, const void *data,
			       size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, data, NULL, data_len);
}

static int spi_flash_write_common(struct spi_flash *flash, const u8 * cmd,
				  size_t cmd_len, const void *buf,
				  size_t buf_len)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timeout = SPI_FLASH_PROG_TIMEOUT;
	int ret;

	if (buf == NULL)
		timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;

	spi_claim_bus(spi);
	ret = spi_flash_cmd_write_enable(flash);
	if (ret < 0) {
		printf("SF: enabling write failed\n");
		return ret;
	}

	ret = spi_flash_cmd_write(spi, cmd, cmd_len, buf, buf_len);
	if (ret < 0) {
		printf("SF: write cmd failed\n");
		return ret;
	}

	ret = spi_flash_wait_till_ready(flash, timeout);
	if (ret < 0) {
		printf("SF: write %s timed out\n",
		       timeout == SPI_FLASH_PROG_TIMEOUT ?
		       "program" : "page erase");
		return ret;
	}
	spi_release_bus(spi);

	return ret;
}

static int read_sr(struct spi_flash *flash, u8 * rs)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_STATUS;
	ret = spi_flash_read_common(flash, &cmd, 1, rs, 1);
	if (ret < 0) {
		printf("SF: fail to read status register\n");
		return ret;
	}

	return 0;
}

static int spi_flash_sr_ready(struct spi_flash *flash)
{
	u8 sr;
	int ret;

	ret = read_sr(flash, &sr);
	if (ret < 0)
		return ret;

	return !(sr & STATUS_WIP);
}

#endif

#if defined(CONFIG_SPI_FLASH_MACRONIX)
static int write_sr(struct spi_flash *flash, u8 ws)
{
	u8 cmd;
	int ret;

	cmd = CMD_WRITE_STATUS;
	ret = spi_flash_write_common(flash, &cmd, 1, &ws, 1);
	if (ret < 0) {
		printf("SF: fail to write status register\n");
		return ret;
	}

	return 0;
}

static int macronix_quad_enable(struct spi_flash *flash)
{
	u8 qeb_status;
	int ret;

	ret = read_sr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_MXIC)
		return 0;

	ret = write_sr(flash, qeb_status | STATUS_QEB_MXIC);
	if (ret < 0)
		return ret;

	/* read SR and check it */
	ret = read_sr(flash, &qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_MXIC))) {
		printf("SF: Macronix SR Quad bit not clear\n");
		return -EINVAL;
	}

	return ret;
}
#endif

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
static int read_cr(struct spi_flash *flash, u8 * rc)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_CONFIG;
	ret = spi_flash_read_common(flash, &cmd, 1, rc, 1);
	if (ret < 0) {
		debug("SF: fail to read config register\n");
		return ret;
	}

	return 0;
}

static int write_cr(struct spi_flash *flash, u8 wc)
{
	u8 data[2];
	u8 cmd;
	int ret;

	ret = read_sr(flash, &data[0]);
	if (ret < 0)
		return ret;

	cmd = CMD_WRITE_STATUS;
	data[1] = wc;
	ret = spi_flash_write_common(flash, &cmd, 1, &data, 2);
	if (ret) {
		debug("SF: fail to write config register\n");
		return ret;
	}

	return 0;
}

static int spansion_quad_enable(struct spi_flash *flash)
{
	u8 qeb_status;
	int ret;

	ret = read_cr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_WINSPAN)
		return 0;

	ret = write_cr(flash, qeb_status | STATUS_QEB_WINSPAN);
	if (ret < 0)
		return ret;

	/* read CR and check it */
	ret = read_cr(flash, &qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_WINSPAN))) {
		printf("SF: Spansion CR Quad bit not clear\n");
		return -EINVAL;
	}

	return ret;
}
#endif

static __maybe_unused int set_quad_mode(struct spi_flash *flash,
					const struct spi_flash_info *info)
{
	switch (JEDEC_MFR(info)) {
#ifdef CONFIG_SPI_FLASH_MACRONIX
	case SPI_FLASH_CFI_MFR_MACRONIX:
		return macronix_quad_enable(flash);
#endif
#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
	case SPI_FLASH_CFI_MFR_SPANSION:
	case SPI_FLASH_CFI_MFR_WINBOND:
		return spansion_quad_enable(flash);
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO
	case SPI_FLASH_CFI_MFR_STMICRO:
		printf("SF: QEB is volatile for %02x flash\n", JEDEC_MFR(info));
		return 0;
#endif
	default:
		printf("SF: Need set QEB func for %02x flash\n",
		       JEDEC_MFR(info));
		return -1;
	}
}

static void spi_flash_addr(struct spi_flash *flash, uint32_t addr,
			   uint8_t * cmd)
{
	cmd[1] = addr >> (flash->addr_width * 8 - 8);
	cmd[2] = addr >> (flash->addr_width * 8 - 16);
	cmd[3] = addr >> (flash->addr_width * 8 - 24);
	cmd[4] = addr >> (flash->addr_width * 8 - 32);
}

int spi_flash_read(u32 offset, size_t len, void *data)
{
	uint8_t cmd[16];
	uint8_t cmdsz;
	uint32_t read_len, read_addr;
	int ret = -1;
	struct spi_flash *flash = &g_spi_flash;

	cmdsz = flash->addr_width + 1 + flash->dummy_byte;
	memset(cmd, 0, sizeof(cmd));

	cmd[0] = flash->read_cmd;
	while (len) {
		read_addr = offset;
		read_len = len;
		spi_flash_addr(flash, read_addr, cmd);
		ret = spi_flash_read_common(flash, cmd, cmdsz, data, read_len);
		if (ret < 0) {
			printf("SF: read failed\n");
			break;
		}

		offset += read_len;
		len -= read_len;
		data += read_len;
	}

	return ret;
}

static int spi_flash_scan(struct spi_flash *flash)
{
	struct spi_slave *spi = (struct spi_slave *)flash->spi;
	const struct spi_flash_info *info = NULL;
	__maybe_unused int ret;
	unsigned int smode = spi->mode;

	/* Id is read by single wire. */
	spi->mode = SPI_RX_SLOW;
	info = spi_flash_read_id(flash);
	spi->mode = smode;
	if (!info)
		return -EINVAL;

	flash->name = info->name;

	if (info->flags & SST_WR)
		flash->flags |= SNOR_F_SST_WR;

	flash->addr_width = 3;

	/* Compute the flash size */
	flash->shift = (flash->dual_flash & SF_DUAL_PARALLEL_FLASH) ? 1 : 0;
	flash->page_size = info->page_size;

	flash->page_size <<= flash->shift;
	flash->sector_size = info->sector_size << flash->shift;
	flash->size = flash->sector_size * info->n_sectors << flash->shift;

#ifdef CONFIG_SPI_FLASH_USE_4K_SECTORS
	/* Compute erase sector and command */
	if (info->flags & SECT_4K) {
		flash->erase_cmd = CMD_ERASE_4K;
		flash->erase_size = 4096 << flash->shift;
	} else
#endif
	{
		flash->erase_cmd = CMD_ERASE_64K;
		flash->erase_size = flash->sector_size;
	}

	/* Now erase size becomes valid sector size */
	flash->sector_size = flash->erase_size;

	/* Look for read commands */
	flash->read_cmd = CMD_READ_ARRAY_FAST;
	if (spi->mode & SPI_RX_SLOW)
		flash->read_cmd = CMD_READ_ARRAY_SLOW;
	else if (spi->mode & SPI_RX_QUAD && info->flags & RD_QUAD)
		flash->read_cmd = CMD_READ_QUAD_OUTPUT_FAST;
	else if (spi->mode & SPI_RX_DUAL && info->flags & RD_DUAL)
		flash->read_cmd = CMD_READ_DUAL_OUTPUT_FAST;

	/* Look for write commands */
	if (info->flags & WR_QPP && spi->mode & SPI_TX_QUAD)
		flash->write_cmd = CMD_QUAD_PAGE_PROGRAM;
	else
		/* Go for default supported write cmd */
		flash->write_cmd = CMD_PAGE_PROGRAM;

#if 0
	/* Set the quad enable bit - only for quad commands */
	if ((flash->read_cmd == CMD_READ_QUAD_OUTPUT_FAST) ||
			(flash->read_cmd == CMD_READ_QUAD_IO_FAST) ||
			(flash->write_cmd == CMD_QUAD_PAGE_PROGRAM)) {
		ret = set_quad_mode(flash, info);
		if (ret) {
			printf("SF: Fail to set QEB for %02x\n",
			       JEDEC_MFR(info));
			return -EINVAL;
		}
	}
#endif

	/* Read dummy_byte: dummy byte is determined based on the
	 * dummy cycles of a particular command.
	 * Fast commands - dummy_byte = dummy_cycles/8
	 * I/O commands- dummy_byte = (dummy_cycles * no.of lines)/8
	 * For I/O commands except cmd[0] everything goes on no.of lines
	 * based on particular command but incase of fast commands except
	 * data all go on single line irrespective of command.
	 */
	switch (flash->read_cmd) {
	case CMD_READ_QUAD_IO_FAST:
		flash->dummy_byte = 2;
		break;
	case CMD_READ_ARRAY_SLOW:
		flash->dummy_byte = 0;
		break;
	default:
		flash->dummy_byte = 1;
	}

	printf("spl SF: Detected %s with page size :%d\nTotal: 0x%x\n",
	       flash->name, flash->page_size, flash->size);

	return 0;
}

static int spi_flash_init(unsigned int spi_num, unsigned int addr_w,
			  unsigned int reset, unsigned int mclk,
			  unsigned int sclk)
{
	struct spi_flash *pflash = &g_spi_flash;
	struct spi_slave *pslave;
	int ret;

#ifdef CONFIG_QSPI_DUAL
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_DUAL);
#elif defined(CONFIG_QSPI_QUAD)
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_QUAD);
#else
	pslave = spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_SLOW);
#endif /* CONFIG_QSPI_DUAL */
	pflash->spi = pslave;

	if (reset > 0)
		spi_flash_reset(pflash);

	ret = spi_flash_scan(pflash);
	if (ret < 0)
		return -1;

	if (addr_w > 0) {
		spi_claim_bus(pslave);
		ret = spi_flash_cmd(pslave, CMD_OP_EX4B, NULL, 0);
		spi_release_bus(pslave);
		pflash->addr_width = 3;
	} else {
		spi_claim_bus(pslave);
		ret = spi_flash_cmd(pslave, CMD_OP_EN4B, NULL, 0);
		spi_release_bus(pslave);
		pflash->addr_width = 4;
	}

	return 0;
}

static unsigned int nor_read_blks(uint64_t lba, uint64_t buf, size_t size)
{
	return spi_flash_read(lba, size, (void *)buf);
}

static void nor_pre_load(struct x2_info_hdr *pinfo,
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

static void nor_load_image(struct x2_info_hdr *pinfo)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;
	__maybe_unused unsigned int read_bytes;

	src_addr = pinfo->other_img[0].img_addr;
	src_len = pinfo->other_img[0].img_size;
	dest_addr = pinfo->other_laddr;

	read_bytes = nor_read_blks((int)src_addr, dest_addr, src_len);

	return;
}

void x2_bootinfo_init(void)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;

	src_addr = 0x10000;
	src_len = 0x200;
	dest_addr = X2_BOOTINFO_ADDR;

	nor_read_blks((int)src_addr, dest_addr, src_len);
}

static int x2_get_dev_mode(void)
{
	uint32_t reg = readl(X2_GPIO_BASE + STRAP_PIN_REG);

	return !!(PIN_DEV_MODE_SEL(reg));
}

void spl_nor_init(void)
{
	int dev_type = x2_get_dev_mode();

	spi_flash_init(0, dev_type, 0, X2_QSPI_MCLK, X2_QSPI_SCLK);
	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = nor_pre_load;
	g_dev_ops.read = nor_read_blks;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = nor_load_image;
	return;
}
#endif
