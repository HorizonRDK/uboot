#include <asm/io.h>
#include "x2_mmc_spl.h"
#include "dw_mmc_spl.h"

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

unsigned int emmc_read_blks(int lba, uint64_t buf, unsigned int size)
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

static void sdio0_pinmux_set(void)
{
	unsigned int regv;
	/* GPIO46 - GPIO49 GPIO50-GPIO58 */
	regv = readl(GPIO3_CFG);
	regv &= 0xFC000000;
	writel(regv, GPIO3_CFG);
}

static int emmc_enumerate(int clk, int bus_width)
{
	emmc_cmd_t cmd;
	int ret, state;

	sdio0_pinmux_set();
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

void emmc_init(dw_mmc_params_t * params)
{
	emmc_ops_t *ops_ptr = config_dw_mmc_ops(params);

	ops = ops_ptr;
	emmc_flags = params->flags;

	emmc_enumerate(params->sclk, params->bus_width);

	printf("eMMC : 0x%x\n", emmc_cid_value >> 24);
}
