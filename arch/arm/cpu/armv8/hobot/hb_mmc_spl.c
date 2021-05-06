#include <asm/io.h>
#include <asm/arch/hb_reg.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch/hb_dev.h>
#include <hb_info.h>
#include <veeprom.h>

#include "hb_mmc_spl.h"
#include "dw_mmc_spl.h"


#ifdef HB_MMC_BOOT
#define HB_EMMC_RE_CFG		(1 << 24)
#define HB_EMMC_RE_WIDTH	(1 << 20)
#define HB_EMMC_RE_SEQ		(1 << 16)
#define HB_EMMC_RE_DIV		(0xFFFF)
#define HB_EMMC_REF_DIV(x)		((x) & 0xF)
#define HB_EMMC_PH_DIV(x)		(((x) >> 4) & 0x7)

static const emmc_ops_t *ops;
static emmc_csd_t emmc_csd;
static unsigned int emmc_flags;
static unsigned int emmc_ocr_value;
static unsigned int emmc_cid_value;

static int is_cmd23_enabled(void)
{
	return (! !(emmc_flags & EMMC_FLAG_CMD23));
}

static int emmc_device_state(void)
{
	emmc_cmd_t cmd;
	int ret;

	do {
		memset(&cmd, 0, sizeof(emmc_cmd_t));
		cmd.cmd_idx = EMMC_CMD13;
		cmd.cmd_arg = EMMC_FIX_RCA << RCA_SHIFT_OFFSET;
		cmd.resp_type = EMMC_RESPONSE_R1;
		ret = ops->send_cmd(&cmd);
		/* Ignore improbable errors in release builds */
		(void)ret;
	} while ((cmd.resp_data[0] & STATUS_READY_FOR_DATA) == 0);

	return EMMC_GET_STATE(cmd.resp_data[0]);
}

static void emmc_set_ext_csd(unsigned int ext_cmd, unsigned int value)
{
	emmc_cmd_t cmd;
	int ret, state;

	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD6;
	cmd.cmd_arg = EXTCSD_WRITE_BYTES | EXTCSD_CMD(ext_cmd) |
	    EXTCSD_VALUE(value) | 1;
	cmd.resp_type = EMMC_RESPONSE_R1B;
	ret = ops->send_cmd(&cmd);

	/* wait to exit PRG state */
	do {
		state = emmc_device_state();
	} while (state == EMMC_STATE_PRG);
	/* Ignore improbable errors in release builds */
	(void)ret;
}

static void emmc_set_ios(int clk, int bus_width)
{
	int ret;

	/* set IO speed & IO bus width */
	if (emmc_csd.spec_vers == 4) {
		emmc_set_ext_csd(CMD_EXTCSD_BUS_WIDTH, bus_width);
		emmc_set_ext_csd(CMD_EXTCSD_HS_TIMING, 0x1);
	}
	ret = ops->set_ios(clk, bus_width);
	/* Ignore improbable errors in release builds */
	(void)ret;
}

static int emmc_enumerate(int clk, int bus_width)
{
	emmc_cmd_t cmd;
	int ret, state;

	ops->init();

	/* CMD0: reset to IDLE */
	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD0;
	ret = ops->send_cmd(&cmd);

	while (1) {
		/* CMD1: get OCR register */
		memset(&cmd, 0, sizeof(emmc_cmd_t));
		cmd.cmd_idx = EMMC_CMD1;
		cmd.cmd_arg = OCR_SECTOR_MODE | OCR_VDD_MIN_2V7 |
		    OCR_VDD_MIN_1V7;
		cmd.resp_type = EMMC_RESPONSE_R3;
		ret = ops->send_cmd(&cmd);
		emmc_ocr_value = cmd.resp_data[0];
		if (emmc_ocr_value & OCR_POWERUP)
			break;
	}

	/* CMD2: Card Identification */
	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD2;
	cmd.resp_type = EMMC_RESPONSE_R2;
	ret = ops->send_cmd(&cmd);
	emmc_cid_value = cmd.resp_data[3];

	/* CMD3: Set Relative Address */
	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD3;
	cmd.cmd_arg = EMMC_FIX_RCA << RCA_SHIFT_OFFSET;
	cmd.resp_type = EMMC_RESPONSE_R1;
	ret = ops->send_cmd(&cmd);

	/* CMD9: CSD Register */
	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD9;
	cmd.cmd_arg = EMMC_FIX_RCA << RCA_SHIFT_OFFSET;
	cmd.resp_type = EMMC_RESPONSE_R2;
	ret = ops->send_cmd(&cmd);
	memcpy(&emmc_csd, &cmd.resp_data, sizeof(cmd.resp_data));

	/* CMD7: Select Card */
	memset(&cmd, 0, sizeof(emmc_cmd_t));
	cmd.cmd_idx = EMMC_CMD7;
	cmd.cmd_arg = EMMC_FIX_RCA << RCA_SHIFT_OFFSET;
	cmd.resp_type = EMMC_RESPONSE_R1;
	ret = ops->send_cmd(&cmd);
	/* wait to TRAN state */
	do {
		state = emmc_device_state();
	} while (state != EMMC_STATE_TRAN);

	emmc_set_ios(clk, bus_width);
	return ret;
}

static void emmc_pre_load(struct hb_info_hdr *pinfo,
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

static unsigned int emmc_read_blks(uint64_t lba,
	uint64_t buf, size_t size)
{
	emmc_cmd_t cmd;
	int ret;

        /* size aligning 512 */
        size = ((size + EMMC_BLOCK_SIZE - 1) / EMMC_BLOCK_SIZE) * EMMC_BLOCK_SIZE;

	flush_dcache_range(buf, buf + size);
	ret = ops->prepare(lba, buf, size);

	if (is_cmd23_enabled()) {
		memset(&cmd, 0, sizeof(emmc_cmd_t));
		/* set block count */
		cmd.cmd_idx = EMMC_CMD23;
		cmd.cmd_arg = (size + EMMC_BLOCK_SIZE) / EMMC_BLOCK_SIZE;
		cmd.resp_type = EMMC_RESPONSE_R1;
		ret = ops->send_cmd(&cmd);

		memset(&cmd, 0, sizeof(emmc_cmd_t));
		cmd.cmd_idx = EMMC_CMD18;
	} else {
		if (size > EMMC_BLOCK_SIZE)
			cmd.cmd_idx = EMMC_CMD18;
		else
			cmd.cmd_idx = EMMC_CMD17;
	}

	/* lba should be a index based on block instead of bytes. */
	lba = lba / EMMC_BLOCK_SIZE;

	if ((emmc_ocr_value & OCR_ACCESS_MODE_MASK) == OCR_BYTE_MODE)
		cmd.cmd_arg = lba * EMMC_BLOCK_SIZE;
	else
		cmd.cmd_arg = lba;

	cmd.resp_type = EMMC_RESPONSE_R1;
	ret = ops->send_cmd(&cmd);
	ret = ops->read(lba, buf, size);

	/* wait buffer empty */
	emmc_device_state();

	if (is_cmd23_enabled() == 0) {
		if (size > EMMC_BLOCK_SIZE) {
			memset(&cmd, 0, sizeof(emmc_cmd_t));
			cmd.cmd_idx = EMMC_CMD12;
			cmd.resp_type = EMMC_RESPONSE_R1;
			ret = ops->send_cmd(&cmd);
		}
	}
	/* Ignore improbable errors in release builds */
	(void)ret;
	return size;
}

static unsigned int emmc_write_blks(uint64_t lba,
	uint64_t buf, size_t size)
{
	emmc_cmd_t cmd;
	int ret;

	/* size aligning 512 */

	size = ((size + EMMC_BLOCK_SIZE - 1) / EMMC_BLOCK_SIZE) * EMMC_BLOCK_SIZE;

	flush_dcache_range(buf, buf + size);

	size = (size + EMMC_BLOCK_MASK) & ~EMMC_BLOCK_MASK;
	ret = ops->prepare(lba, buf, size);

	if (is_cmd23_enabled()) {
		/* set block count */
		memset(&cmd, 0, sizeof(emmc_cmd_t));
		cmd.cmd_idx = EMMC_CMD23;
		cmd.cmd_arg = (size + EMMC_BLOCK_SIZE) / EMMC_BLOCK_SIZE;
		cmd.resp_type = EMMC_RESPONSE_R1;
		ret = ops->send_cmd(&cmd);

		memset(&cmd, 0, sizeof(emmc_cmd_t));
		cmd.cmd_idx = EMMC_CMD25;
	} else {
		memset(&cmd, 0, sizeof(emmc_cmd_t));
		if (size > EMMC_BLOCK_SIZE)
			cmd.cmd_idx = EMMC_CMD25;
		else
			cmd.cmd_idx = EMMC_CMD24;
	}

	/* lba should be a index based on block instead of bytes. */
	lba = lba / EMMC_BLOCK_SIZE;

	if ((emmc_ocr_value & OCR_ACCESS_MODE_MASK) == OCR_BYTE_MODE)
		cmd.cmd_arg = lba * EMMC_BLOCK_SIZE;
	else
		cmd.cmd_arg = lba;

	cmd.resp_type = EMMC_RESPONSE_R1;
	ret = ops->send_cmd(&cmd);
	ret = ops->write(lba, buf, size);

	/* wait buffer empty */
	emmc_device_state();

	if (is_cmd23_enabled() == 0) {
		if (size > EMMC_BLOCK_SIZE) {
			memset(&cmd, 0, sizeof(emmc_cmd_t));
			cmd.cmd_idx = EMMC_CMD12;
			cmd.resp_type = EMMC_RESPONSE_R1B;
			ret = ops->send_cmd(&cmd);
		}
	}

	/* Ignore improbable errors in release builds */
	(void)ret;

	return size;
}

static unsigned int start_sector = VEEPROM_START_SECTOR;
static char buffer[512];

int veeprom_read(int offset, char *buf, int size)
{
	uint64_t cur_sector = 0;
	uint64_t n = 0;

	cur_sector = start_sector + (offset / EMMC_BLOCK_SIZE);
	memset(buffer, 0, sizeof(buffer));

	n = emmc_read_blks(cur_sector * 512, (uint64_t) buffer, 0x200);
	flush_cache((ulong)buffer, 512);
	if (n != 0x200) {
		printf("Error: read sector %lld fail\n", cur_sector);
		return -1;
	}

	memcpy(buf, buffer + offset, size);
	return 0;
}

int veeprom_write(int offset, const char *buf, int size)
{
	uint64_t cur_sector = 0;
	uint64_t n = 0;

	cur_sector = start_sector + (offset / EMMC_BLOCK_SIZE);
	memset(buffer, 0, sizeof(buffer));

	n = emmc_read_blks(cur_sector * 512, (uint64_t)buffer, 0x200);
	flush_cache((ulong)buffer, 512);
	if (n != 0x200) {
		printf("Error: read sector %lld fail\n", cur_sector);
		return -1;
	}

	memcpy(buffer + offset, buf, size);

	n = emmc_write_blks(cur_sector * 512, (uint64_t)buffer, 0x200);
	if (n != 0x200) {
		printf("Error: write sector %lld fail\n", cur_sector);
		return -1;
	}

	return 0;
}

static void ota_update_failed_output(char* boot_reason)
{
	printf("*************************************************\n");
	printf("Error: update %s failed! \n", boot_reason);
	printf("*************************************************\n");
}

static void ota_get_update_status(char* up_flag, char* partstatus,
	char* boot_reason)
{
	veeprom_read(VEEPROM_UPDATE_FLAG_OFFSET, up_flag,
			VEEPROM_UPDATE_FLAG_SIZE);

	veeprom_read(VEEPROM_ABMODE_STATUS_OFFSET, partstatus,
			VEEPROM_ABMODE_STATUS_SIZE);

	veeprom_read(VEEPROM_RESET_REASON_OFFSET, boot_reason,
			VEEPROM_RESET_REASON_SIZE);
}

static bool ota_spl_update_check(void) {
	char boot_reason[64] = { 0 } ;
	char up_flag, partstatus, count;
	bool flash_success, first_try, app_success;
	bool uboot_bak = 0;
	bool uboot_status = 0;
	int boot_flag = 0;

	ota_get_update_status(&up_flag, &partstatus, boot_reason);
	printf("boot reason: %s\n", boot_reason);

	uboot_status = (partstatus >> 1) & 0x1;
	boot_flag = uboot_status;

	if (strcmp(boot_reason, "normal") == 0)  {
		/*
		 * During normal startup, use backup partitions
		 * if 10 consecutive startup failures occur
		 */
		veeprom_read(VEEPROM_COUNT_OFFSET, &count, VEEPROM_COUNT_SIZE);
		if (count > 0) {
			count = count - 1;
			veeprom_write(VEEPROM_COUNT_OFFSET, &count, VEEPROM_COUNT_SIZE);
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
			veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
					VEEPROM_UPDATE_FLAG_SIZE);
		} else if(app_success == 0) {
				boot_flag = uboot_bak;
		}

		if (boot_flag == uboot_bak) {
			up_flag = up_flag & 0x7;
			veeprom_write(VEEPROM_UPDATE_FLAG_OFFSET, &up_flag,
				VEEPROM_UPDATE_FLAG_SIZE);

			ota_update_failed_output(boot_reason);
		}
	}

	return boot_flag;
}

static void emmc_load_image(struct hb_info_hdr *pinfo)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;
	unsigned int __maybe_unused read_bytes;
        bool boot_flag = 0;
        char upmode[16] = { 0 };

	veeprom_read(VEEPROM_UPDATE_MODE_OFFSET, upmode,
			VEEPROM_UPDATE_MODE_SIZE);

	/* When upmode is AB, support update status checking */
	if ((strcmp(upmode, "AB") == 0) || (strcmp(upmode, "golden") == 0))
		boot_flag = ota_spl_update_check();

	src_addr = pinfo->other_img[boot_flag].img_addr;
	src_len = pinfo->other_img[boot_flag].img_size;

	dest_addr = pinfo->other_laddr;
	read_bytes = emmc_read_blks((int)src_addr, dest_addr, src_len);

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

	emmc_read_blks((int)src_addr, dest_addr, src_len);
}

static void emmc_init(dw_mmc_params_t * params)
{
	emmc_ops_t *ops_ptr = config_dw_mmc_ops(params);

	ops = ops_ptr;
	emmc_flags = params->flags;

	emmc_enumerate(params->sclk, params->bus_width);
}

void spl_emmc_init(unsigned int emmc_config)
{
	dw_mmc_params_t params;
	unsigned int mclk;
	unsigned int width = EMMC_BUS_WIDTH_4;
	unsigned int ref_div = 3;
	unsigned int ph_div = 7;
	unsigned int val = (ph_div << 4 | ref_div);
	unsigned int sclk;

	if (emmc_config & HB_EMMC_RE_CFG) {
		val = (emmc_config & HB_EMMC_RE_DIV);
		width = (emmc_config & HB_EMMC_RE_WIDTH) ? EMMC_BUS_WIDTH_4 :
			EMMC_BUS_WIDTH_8;
		ref_div = HB_EMMC_REF_DIV(val);
		ph_div = HB_EMMC_PH_DIV(val);
	}

	mclk = 1500000000 / (ref_div + 1) / (ph_div + 1);
#ifdef CONFIG_X2_PM
	sclk = 10000000;
	width = EMMC_BUS_WIDTH_1;
#else
	sclk = mclk;
#endif /* CONFIG_X2_PM */

	/*
	 * The divider will be updated
	 * after PERIPLL has been set to 1500M.
	 */
	writel(val, HB_SD0_CCLK_CTRL);

	switch (width) {
		case EMMC_BUS_WIDTH_1:
			val = 1;
			break;

		case EMMC_BUS_WIDTH_8:
			val = 8;
			break;

		default:
			val = 4;
			break;
	}
	printf("emmc: width = %d, mclk = %d, sclk = %d\n", val, mclk, sclk);

	val = readl(GPIO_BASE + 0x30);
	val &= ~0x3ffffff;
	writel(val, GPIO_BASE + 0x30);

	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = SDIO0_BASE;
	params.bus_width = width;
	params.clk_rate = mclk;
	params.sclk = mclk - 500000;
	//?params.sclk = sclk;
	params.flags = 0;

	emmc_init(&params);

	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = emmc_pre_load;
	g_dev_ops.read = emmc_read_blks;
	g_dev_ops.erase = NULL;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = emmc_load_image;
#ifdef CONFIG_X2_PM
	g_dev_ops.write = emmc_write_blks;
#endif /* CONFIG_X2_PM */

	return;
}

#endif /* HB_MMC_BOOT */
