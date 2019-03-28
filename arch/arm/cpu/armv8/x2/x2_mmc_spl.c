
#include <asm/io.h>
#include <asm/arch/x2_reg.h>
#include <asm/arch/x2_sysctrl.h>
#include <asm/arch/x2_dev.h>

#include "x2_mmc_spl.h"
#include "dw_mmc_spl.h"
#include "x2_info.h"

#ifdef CONFIG_X2_MMC_BOOT

#define X2_EMMC_RE_CFG		(1 << 24)
#define X2_EMMC_RE_WIDTH	(1 << 20)
#define X2_EMMC_RE_SEQ		(1 << 16)
#define X2_EMMC_RE_DIV		(0xFFFF)
#define X2_EMMC_REF_DIV(x)		((x) & 0xF)
#define X2_EMMC_PH_DIV(x)		(((x) >> 4) & 0x7)

static const emmc_ops_t *ops;
static emmc_csd_t emmc_csd;
unsigned int emmc_flags;
unsigned int emmc_ocr_value;
unsigned int emmc_cid_value;

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
	if (emmc_csd.spec_vers == 4)
		emmc_set_ext_csd(CMD_EXTCSD_BUS_WIDTH, bus_width);
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

static void emmc_pre_load(struct x2_info_hdr *pinfo,
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

static unsigned int emmc_read_blks(int lba,
	uint64_t buf, size_t size)
{
	emmc_cmd_t cmd;
	int ret;

	flush_dcache_range(buf, size);
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

static void emmc_load_image(struct x2_info_hdr *pinfo)
{
	unsigned int src_addr;
	unsigned int src_len;
	unsigned int dest_addr;
	unsigned int read_bytes;

	src_addr = pinfo->other_img[0].img_addr;
	src_len = pinfo->other_img[0].img_size;
	dest_addr = pinfo->other_laddr;

	read_bytes = emmc_read_blks((int)src_addr, dest_addr, src_len);

	return;
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
	unsigned int ref_div = 14;
	unsigned int ph_div = 7;
	unsigned int val = X2_EMMC_REF_DIV(ref_div) | X2_EMMC_PH_DIV(ph_div);

	if (emmc_config & X2_EMMC_RE_CFG) {
		val = (emmc_config & X2_EMMC_RE_DIV);
		width = (emmc_config & X2_EMMC_RE_WIDTH) ? EMMC_BUS_WIDTH_4 :
			EMMC_BUS_WIDTH_8;
		ref_div = X2_EMMC_REF_DIV(val);
		ph_div = X2_EMMC_PH_DIV(val);
	}

	mclk = 1500000000 / (ref_div + 1) / (ph_div + 1);

	/*
	 * The divider will be updated
	 * after PERIPLL has been set to 1500M.
	 */
	writel(val, X2_SD0_CCLK_CTRL);

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
	printf("emmc: width = %d, mclk = %d\n", val, mclk);

	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = SDIO0_BASE;
	params.bus_width = width;
	params.clk_rate = mclk;
	params.sclk = mclk;
	params.flags = 0;

	emmc_init(&params);

	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = emmc_pre_load;
	g_dev_ops.read = emmc_read_blks;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = emmc_load_image;

	return;
}

#endif /* CONFIG_X2_MMC_BOOT */
