#ifdef HB_NOR_BOOT
#include <asm/io.h>
#include <asm/arch/hb_dev.h>
#include <asm/arch/hb_pinmux.h>
#include <spi.h>
#include <veeprom.h>
#include <hb_info.h>
#include "hb_nor_spl.h"
#include "hb_spi_spl.h"

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

#define NOR_PAGE_SIZE	(512)
#define NOR_SECTOR_SIZE	(64*1024)

/* Dual SPI flash memories - see SPI_COMM_DUAL_... */
enum spi_dual_flash {
	SF_SINGLE_FLASH	= 0,
	SF_DUAL_STACKED_FLASH	= BIT(0),
	SF_DUAL_PARALLEL_FLASH	= BIT(1),
};

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

#define SPL_RECONF_SPI_FREQ(d, v)          ((d) ? (d) : (v))

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
	{"gd25lq128d", INFO(0xC86018, 0x0, 64 * 1024, 256, RD_FULL | WR_QPP | SECT_4K)},
	{"gd25lq256d", INFO(0xc86019, 0x0, 64 * 1024, 512, RD_FULL | WR_QPP | SECT_4K)},
	{"w25q256jw", INFO(0xef6019, 0x0, 64 * 1024, 512, RD_FULL | WR_QPP | SECT_4K)},
	{"is25wp512", INFO(0x9d701a, 0x0, 64 * 1024,   1024, RD_FULL | WR_QPP) },
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

	ret = spl_spi_xfer(spi, cmd_len * 8, cmd, NULL, flags | SPI_XFER_CMD);
	if (ret) {
		printf("SF: Failed to send command (%ld bytes): %d\n", cmd_len,
		       ret);
	} else if (data_len != 0) {
		ret = spl_spi_xfer(spi, data_len * 8, data_out, data_in,
				 SPI_XFER_END);
		if (ret)
			printf("SF: Failed to transfer %ld bytes of data: %d\n",
			       data_len, ret);
	}

	return ret;
}

static int spi_flash_cmd(struct spi_slave *spi, uint8_t cmd, void *response,
			 size_t len)
{
	return spi_flash_read_write(spi, &cmd, 1, NULL, response, len);
}

static int spi_flash_reset(struct spi_flash *flash)
{
	int ret;
	struct spi_slave *spi = (struct spi_slave *)flash->spi;

	spl_spi_claim_bus(spi);
	ret = spi_flash_cmd(flash->spi, CMD_RESET_EN, NULL, 0);
	spl_spi_release_bus(spi);

	udelay(2);

	spl_spi_claim_bus(spi);
	ret = spi_flash_cmd(flash->spi, CMD_RESET, NULL, 0);
	spl_spi_release_bus(spi);

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

	spl_spi_claim_bus(spi);
	tmp = spi_flash_cmd(flash->spi, CMD_READ_ID, id, SPI_FLASH_MAX_ID_LEN);
	spl_spi_release_bus(spi);

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

	spl_spi_claim_bus(spi);
	ret = spi_flash_read_write(spi, cmd, cmd_len, NULL, data, data_len);
	spl_spi_release_bus(spi);
	if (ret < 0) {
		printf("SF: read cmd failed\n");
		return ret;
	}

	return data_len;
}

static int read_sr(struct spi_flash *flash, u8 * rs)
{
	int ret;
	u8 cmd = CMD_READ_STATUS;

	ret = spi_flash_read_common(flash, &cmd, 1, rs, 1);
	if (ret < 0) {
		printf("SF: fail to read status register\n");
		return ret;
	}

	return 0;
}

static inline int spi_flash_cmd_write_enable(struct spi_flash *flash)
{
	return spi_flash_cmd(flash->spi, CMD_WRITE_ENABLE, NULL, 0);
}

/* Enable writing on the SPI flash */
static inline int spi_flash_sr_ready(struct spi_flash *flash)
{
	u8 sr;
	int ret;

	ret = read_sr(flash, &sr);
	if (ret < 0)
		return ret;

	return !(sr & STATUS_WIP);
}

static int spi_flash_wait_till_ready(struct spi_flash *flash,
	unsigned long timeout)
{
	unsigned long timebase;
	int ret = -ETIMEDOUT;

	timebase = get_timer(0);

	while (get_timer(timebase) < timeout) {
		ret = spi_flash_sr_ready(flash);
		if (ret < 0) {
			break;
		}

		if (ret) {
			return 0;
		}
	}

	return ret;
}

static int spi_flash_cmd_write(struct spi_slave *spi, const u8 * cmd,
	size_t cmd_len, const void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, data, NULL, data_len);
}

static void spi_flash_addr(struct spi_flash *flash, uint32_t addr,
	uint8_t * cmd)
{
	cmd[1] = addr >> (flash->addr_width * 8 - 8);
	cmd[2] = addr >> (flash->addr_width * 8 - 16);
	cmd[3] = addr >> (flash->addr_width * 8 - 24);
	cmd[4] = addr >> (flash->addr_width * 8 - 32);
}

int spi_flash_read(u32 offset, void *data, size_t len)
{
	uint8_t cmd[8];
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
			break;
		}

		offset += read_len;
		len -= read_len;
		data += read_len;
	}

	return ret;
}

//#ifdef CONFIG_X2_PM
static int spi_flash_write_common(struct spi_flash *flash, const u8 * cmd,
	size_t cmd_len, const void *buf, size_t buf_len)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timeout = SPI_FLASH_PROG_TIMEOUT;
	int ret;

	if (buf == NULL)
		timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;

	spl_spi_claim_bus(spi);
	ret = spi_flash_cmd_write_enable(flash);
	spl_spi_release_bus(spi);
	if (ret < 0) {
		printf("SF: enabling write failed\n");
		return ret;
	}

	spl_spi_claim_bus(spi);
	ret = spi_flash_cmd_write(spi, cmd, cmd_len, buf, buf_len);
	spl_spi_release_bus(spi);
	if (ret < 0) {
		return ret;
	}

	spl_spi_claim_bus(spi);
	ret = spi_flash_wait_till_ready(flash, timeout);
	spl_spi_release_bus(spi);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int spi_flash_write(u32 offset, void *data, size_t len)
{
	struct spi_flash *flash = &g_spi_flash;
	u32 byte_addr, page_size;
	u32 write_addr;
	size_t chunk_len, actual;
	u8 cmd[8];
	u8 cmdsz;
	int ret = -1;

	page_size = flash->page_size;

	cmdsz = flash->addr_width + 1;
	cmd[0] = flash->write_cmd;

	for (actual = 0; actual < len; actual += chunk_len) {
		write_addr = offset;

		byte_addr = offset % page_size;
		chunk_len = min(len - actual, (size_t)(page_size - byte_addr));

		spi_flash_addr(flash, write_addr, cmd);
		ret = spi_flash_write_common(flash, cmd, cmdsz,
			(const u8*)data + actual, chunk_len);
		if (ret < 0) {
			break;
		}

		offset += chunk_len;
	}

	return ret;
}

static int spi_flash_erase(u32 offset, size_t len)
{
	struct spi_flash *flash = &g_spi_flash;
	u32 erase_size, erase_addr;
	u8 cmd[8];
	size_t cmd_size = flash->addr_width + 1;
	int ret = -1;

	erase_size = flash->erase_size;
	if (offset % erase_size || len % erase_size) {
		return -1;
	}

	cmd[0] = flash->erase_cmd;
	while (len) {
		erase_addr = offset;

		spi_flash_addr(flash, erase_addr, cmd);

		ret = spi_flash_write_common(flash, cmd, cmd_size, NULL, 0);
		if (ret < 0) {
			break;
		}

		offset += erase_size;
		len -= erase_size;
	}

	return ret;
}
//#endif /* CONFIG_X2_PM */

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

	flash->addr_width = 3;

	/* Compute the flash size */
	flash->shift = (flash->dual_flash & SF_DUAL_PARALLEL_FLASH) ? 1 : 0;
	flash->page_size = info->page_size;

	flash->page_size <<= flash->shift;
	flash->sector_size = info->sector_size << flash->shift;
	flash->size = flash->sector_size * info->n_sectors << flash->shift;

	flash->erase_cmd = CMD_ERASE_64K;
	flash->erase_size = flash->sector_size;

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

	printf("spl: Detected %s with page size :%d\nTotal: 0x%x\n",
	       flash->name, flash->page_size, flash->size);

	return 0;
}

static int spi_flash_init(unsigned int spi_num, unsigned int addr_w,
			  unsigned int reset, unsigned int sclk, u8 mode)
{
	struct spi_flash *pflash = &g_spi_flash;
	struct spi_slave *pslave;
	int ret;
	unsigned int mclk;

	memset((void *)pflash, 0, sizeof(*pflash));

	if (mode == HB_QSPI_DUAL_MODE) {
		pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_DUAL);
	} else if (mode == HB_QSPI_SLOW_MODE) {
		pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_SLOW);
	} else {
		pslave = spl_spi_setup_slave(spi_num, QSPI_DEV_CS0, 0, SPI_RX_QUAD);
	}
	pflash->spi = pslave;
	mclk = spl_spi_set_speed(pslave, sclk);

	if (reset > 0)
		spi_flash_reset(pflash);

	ret = spi_flash_scan(pflash);
	if (ret < 0)
		return -1;

	if (addr_w > 0) {
		spl_spi_claim_bus(pslave);
		ret = spi_flash_cmd(pslave, CMD_OP_EX4B, NULL, 0);
		spl_spi_release_bus(pslave);
		pflash->addr_width = 3;
	} else {
		spl_spi_claim_bus(pslave);
		ret = spi_flash_cmd(pslave, CMD_OP_EN4B, NULL, 0);
		spl_spi_release_bus(pslave);
		pflash->addr_width = 4;
	}

	printf("spi switch to %d HZ, mclk %d HZ, dev_m=%d, rest=%d, mode=0x%x\n",
		sclk, mclk, addr_w, reset, mode);

	return 0;
}

static unsigned int nor_read_blks(uint64_t lba, uint64_t buf, size_t size)
{
	return spi_flash_read(lba, (void *)buf, size);
}

#ifdef CONFIG_X2_PM
static inline uint32_t nor_write_blks(uint32_t lba, const uintptr_t buf, size_t size)
{
	int ret;

	size = (size + 255) & 255;

	ret = spi_flash_write(lba, (void *)buf, size);
	if (ret < 0) {
		return 0;
	}

	return size;
}

static inline int nor_erase_blks(uint32_t lba, size_t size)
{
	return spi_flash_erase(lba, size);
}
#endif /* CONFIG_X2_PM */

static void nor_pre_load(struct hb_info_hdr *pinfo,
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

static unsigned int nor_start_sector = NOR_VEEPROM_START_SECTOR;
static char nor_buffer[64];

int nor_veeprom_read(int offset, char *buf, int size)
{
	uint64_t cur_sector = 0;

	cur_sector = nor_start_sector + (offset / NOR_PAGE_SIZE);

	memset(nor_buffer, 0, sizeof(nor_buffer));

	spi_flash_read(cur_sector * NOR_PAGE_SIZE, (void *)nor_buffer,
		sizeof(nor_buffer));
	flush_cache((ulong)nor_buffer, sizeof(nor_buffer));

	memcpy(buf, nor_buffer + offset, size);

	return 0;
}

int nor_veeprom_write(int offset, const char *buf, int size)
{
	uint64_t cur_sector = 0;

	cur_sector = nor_start_sector + (offset / NOR_PAGE_SIZE);
	int operate_count = 0;
	memset(nor_buffer, 0, sizeof(nor_buffer));

	/* read nor flash */
	spi_flash_read(cur_sector * NOR_PAGE_SIZE, (void *)nor_buffer,
		sizeof(nor_buffer));
	flush_cache((ulong)nor_buffer, sizeof(nor_buffer));

	memcpy(nor_buffer + offset, buf, size);
	buf += operate_count;

	/* erase nor flash */
	spi_flash_erase(cur_sector * NOR_PAGE_SIZE, NOR_SECTOR_SIZE);

	/* write nor flash */
	spi_flash_write(cur_sector * NOR_PAGE_SIZE, (void *)nor_buffer,
		sizeof(nor_buffer));

	return 0;
}

static void ota_nor_get_update_status(char* up_flag, char* partstatus,
	char* boot_reason)
{
	nor_veeprom_read(VEEPROM_UPDATE_FLAG_OFFSET, up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

	nor_veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	nor_veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
}

static bool ota_nor_spl_update_check(void) {
	char boot_reason[64] = { 0 };
	char up_flag, partstatus, count;
	bool flash_success, first_try, app_success;
	bool uboot_bak = 0;
	bool uboot_status = 0;
	int boot_flag = 0;

	ota_nor_get_update_status(&up_flag, &partstatus, boot_reason);
	printf("boot reason: %s \n", boot_reason);

	uboot_status = (partstatus >> 1) & 0x1;
	boot_flag = uboot_status;

	if (strcmp(boot_reason, "normal") == 0)  {
		/*
		 * During normal startup, use backup partitions
		 * if 10 consecutive startup failures occur
		 */
		nor_veeprom_read(VEEPROM_COUNT_OFFSET, &count, VEEPROM_COUNT_SIZE);
		if (count > 0) {
			count = count - 1;
			nor_veeprom_write(VEEPROM_COUNT_OFFSET, &count, VEEPROM_COUNT_SIZE);
		}

		if (count <= 0) {
			printf("Error: Failed times more than 10, using backup partitions\n");
			boot_flag = uboot_status^1;
		}
	} else if ((strcmp(boot_reason, "recovery") == 0)) {
		/* using ubootbak partition entry recovery mode */
		boot_flag = uboot_status^1;
	} else if((strcmp(boot_reason, "uboot") == 0) ||
		(strcmp(boot_reason, "all") == 0) ) {
		flash_success = (up_flag >> 2) & 0x1;
		first_try = (up_flag >> 1) & 0x1;
		app_success = up_flag & 0x1;
		uboot_bak = uboot_status^1;

		if (flash_success == 0)
			boot_flag = uboot_bak;
		else if (first_try == 1) {
			up_flag = up_flag & 0xd;
			nor_veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
					VEEPROM_UPDATE_FLAG_SIZE);
		} else if(app_success == 0) {
				boot_flag = uboot_bak;
		}

		if (boot_flag == uboot_bak) {
			up_flag = up_flag & 0x7;
			nor_veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
				VEEPROM_UPDATE_FLAG_SIZE);

			printf("Error: update %s failed! \n", boot_reason);
		}
	}

	return boot_flag;
}

static void nor_load_image(struct hb_info_hdr *pinfo)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;
	__maybe_unused unsigned int read_bytes;
	bool boot_flag = 0;
	char upmode[16] = { 0 };

	nor_veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	/* When upmode is AB, support update status checking */
	if ((strcmp(upmode, "golden") == 0))
		boot_flag = ota_nor_spl_update_check();

	src_addr = pinfo->other_img[boot_flag].img_addr;
	src_len = pinfo->other_img[boot_flag].img_size;
	dest_addr = pinfo->other_laddr;

	read_bytes = nor_read_blks((int)src_addr, dest_addr, src_len);

	return;
}

void hb_bootinfo_init(void)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;

	src_addr = 0x10000;
	src_len = 0x200;
	dest_addr = HB_BOOTINFO_ADDR;

	nor_read_blks((int)src_addr, dest_addr, src_len);
}

#ifdef CONFIG_X2_PM
static int hb_get_dev_mode(void)
{
	uint32_t reg = readl(HB_GPIO_BASE +X2_STRAP_PIN_REG);

	return !!(PIN_DEV_MODE_SEL(reg));
}
#endif

void spl_nor_init(unsigned int recfg)
{
	int dev_mode = hb_pin_get_dev_mode();
	int rest = hb_pin_get_reset_sf();
	u32 freq = HB_QSPI_SCLK;
	u8 mode = 0;

	if (recfg & HB_QSPI_RE_MODE) {
		mode = recfg & HB_QSPI_RE_MODE_MASK;
	}

	if (recfg & HB_QSPI_RE_FRQ) {
		freq = SPL_RECONF_SPI_FREQ(recfg >> HB_QSPI_RE_FRQ_SHIFT, freq);
	}
	spi_flash_init(0, dev_mode, rest, freq, mode);

	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = nor_pre_load;
	g_dev_ops.read = nor_read_blks;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = nor_load_image;

#ifdef CONFIG_X2_PM
	g_dev_ops.write = nor_write_blks;
	g_dev_ops.erase = nor_erase_blks;
#endif /* CONFIG_X2_PM */

	return;
}
#endif
