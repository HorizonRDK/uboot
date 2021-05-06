#include <asm/io.h>
#include "hb_mmc_spl.h"
#include "dw_mmc_spl.h"

#ifdef HB_MMC_BOOT
#define DWMMC_CTRL				(0x00)
#define CTRL_IDMAC_EN			(1 << 25)
#define CTRL_DMA_EN				(1 << 5)
#define CTRL_INT_EN				(1 << 4)
#define CTRL_DMA_RESET			(1 << 2)
#define CTRL_FIFO_RESET			(1 << 1)
#define CTRL_RESET				(1 << 0)
#define CTRL_RESET_ALL			(CTRL_DMA_RESET | CTRL_FIFO_RESET | \
									CTRL_RESET)

#define DWMMC_PWREN				(0x04)
#define DWMMC_CLKDIV			(0x08)
#define DWMMC_CLKSRC			(0x0c)
#define DWMMC_CLKENA			(0x10)
#define DWMMC_TMOUT				(0x14)
#define DWMMC_CTYPE				(0x18)
#define CTYPE_8BIT				(1 << 16)
#define CTYPE_4BIT				(1)
#define CTYPE_1BIT				(0)

#define DWMMC_BLKSIZ			(0x1c)
#define DWMMC_BYTCNT			(0x20)
#define DWMMC_INTMASK			(0x24)
#define INT_EBE					(1 << 15)
#define INT_SBE					(1 << 13)
#define INT_HLE					(1 << 12)
#define INT_FRUN				(1 << 11)
#define INT_DRT					(1 << 9)
#define INT_RTO					(1 << 8)
#define INT_DCRC				(1 << 7)
#define INT_RCRC				(1 << 6)
#define INT_RXDR				(1 << 5)
#define INT_TXDR				(1 << 4)
#define INT_DTO					(1 << 3)
#define INT_CMD_DONE			(1 << 2)
#define INT_RE					(1 << 1)

#define DWMMC_CMDARG			(0x28)
#define DWMMC_CMD				(0x2c)
#define CMD_START				(1 << 31)
#define CMD_USE_HOLD_REG		(1 << 29)	/* 0 if SDR50/100 */
#define CMD_UPDATE_CLK_ONLY		(1 << 21)
#define CMD_SEND_INIT			(1 << 15)
#define CMD_STOP_ABORT_CMD		(1 << 14)
#define CMD_WAIT_PRVDATA_COMPLETE	(1 << 13)
#define CMD_WRITE				(1 << 10)
#define CMD_DATA_TRANS_EXPECT	(1 << 9)
#define CMD_CHECK_RESP_CRC		(1 << 8)
#define CMD_RESP_LEN			(1 << 7)
#define CMD_RESP_EXPECT			(1 << 6)
#define CMD(x)					(x & 0x3f)

#define DWMMC_RESP0				(0x30)
#define DWMMC_RESP1				(0x34)
#define DWMMC_RESP2				(0x38)
#define DWMMC_RESP3				(0x3c)
#define DWMMC_RINTSTS			(0x44)
#define DWMMC_STATUS			(0x48)

#define STATUS_FIFO_EMPTY		(1 << 2)
#define STATUS_FIFO_FULL		(1 << 3)
#define STATUS_DATA_BUSY		(1 << 9)
#define STATUS_FIFO_SHIFT		(17)
#define STATUS_FIFO_MASK		(0x1FFF)

#define DWMMC_FIFOTH			(0x4c)
#define FIFOTH_TWMARK(x)		((x) & 0xfff)
#define FIFOTH_RWMARK_SHIFT		(16)
#define FIFOTH_RWMARK_MASK		(0xfff << FIFOTH_RWMARK_SHIFT)
#define FIFOTH_RWMARK(x)		(((x) & 0xfff) << 16)
#define FIFOTH_DMA_BURST_SIZE(x)	(((x) & 0x7) << 28)

#define DWMMC_DEBNCE			(0x64)
#define DWMMC_BMOD				(0x80)
#define BMOD_ENABLE				(1 << 7)
#define BMOD_FB					(1 << 1)
#define BMOD_SWRESET			(1 << 0)

#define DWMMC_UHS				(0x74)
#define VDD_VOLT_1_8V			(1 << 0)
#define VDD_VOLT_3_3V			(0)

#define DWMMC_DBADDR			(0x88)
#define DWMMC_IDSTS				(0x8c)
#define DWMMC_IDINTEN			(0x90)
#define DWMMC_CARDTHRCTL		(0x100)
#define CARDTHRCTL_RD_THR(x)	((x & 0xfff) << 16)
#define CARDTHRCTL_RD_THR_EN	(1 << 0)

#define DWMMC_DATA				(0x200)

#define IDMAC_DES0_DIC			(1 << 1)
#define IDMAC_DES0_LD			(1 << 2)
#define IDMAC_DES0_FS			(1 << 3)
#define IDMAC_DES0_CH			(1 << 4)
#define IDMAC_DES0_ER			(1 << 5)
#define IDMAC_DES0_CES			(1 << 30)
#define IDMAC_DES0_OWN			(1 << 31)
#define IDMAC_DES1_BS1(x)		((x) & 0x1fff)
#define IDMAC_DES2_BS2(x)		(((x) & 0x1fff) << 13)

#define DWMMC_DMA_MAX_BUFFER_SIZE	(4096)
#define DWMMC_8BIT_MODE			(1 << 6)
#define TIMEOUT					(0x8000000)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

struct dw_idmac_desc {
	uint32_t des0;
	uint32_t des1;
	uint32_t des2;
	uint32_t des3;
};

static void dw_init(void);
static int dw_send_cmd(emmc_cmd_t * cmd);
static int dw_set_ios(int clk, int width);
static int dw_prepare(uint64_t lba, uint64_t buf, unsigned int size);
static int dw_read(uint64_t lba, uint64_t buf, unsigned int size);
static int dw_write(uint64_t lba, uint64_t buf, unsigned int size);

static emmc_ops_t dw_mmc_ops = {
	.init = dw_init,
	.send_cmd = dw_send_cmd,
	.set_ios = dw_set_ios,
	.prepare = dw_prepare,
	.read = dw_read,
	.write = dw_write,
};

static dw_mmc_params_t dw_params;

static void dw_update_clk(void)
{
	unsigned int data;
	uint64_t base = dw_params.reg_base;

	writel(CMD_WAIT_PRVDATA_COMPLETE | CMD_UPDATE_CLK_ONLY | CMD_START,
	       (base + DWMMC_CMD));
	while (1) {
		data = readl(base + DWMMC_CMD);
		if ((data & CMD_START) == 0)
			break;
		data = readl(base + DWMMC_RINTSTS);
	}
}

static void dw_set_clk(int clk)
{
	unsigned int ret;
	uint64_t base;
	int div;

	if (dw_params.clk_rate == clk) {
		div = 0;	/* bypass mode */
	} else {
		div = DIV_ROUND_UP(dw_params.clk_rate, 2 * clk);
	}
	base = dw_params.reg_base;

	/* wait until controller is idle */
	do {
		ret = readl(base + DWMMC_STATUS);
	} while (ret & STATUS_DATA_BUSY);

	/* disable clock before change clock rate */
	writel(0x0, base + DWMMC_CLKENA);
	dw_update_clk();

	writel(div, base + DWMMC_CLKDIV);
	dw_update_clk();

	/* enable clock */
	writel(0x10001, base + DWMMC_CLKENA);
	writel(0, base + DWMMC_CLKSRC);
	dw_update_clk();
}

static void dw_init(void)
{
	unsigned int ret, data;
	unsigned int fifo_size;
	uint64_t base;

	base = dw_params.reg_base;
	writel(1, base + DWMMC_PWREN);

	writel(CTRL_RESET_ALL, base + DWMMC_CTRL);
	do {
		ret = readl(base + DWMMC_CTRL);
	} while (ret);

	writel(VDD_VOLT_1_8V, base + DWMMC_UHS);
	writel(~0, base + DWMMC_RINTSTS);
	writel(0x0, base + DWMMC_INTMASK);
	writel(~0, base + DWMMC_TMOUT);
	writel(EMMC_BLOCK_SIZE, base + DWMMC_BLKSIZ);
	writel(0xffffff, base + DWMMC_DEBNCE);

	writel(BMOD_SWRESET, base + DWMMC_BMOD);
	do {
		ret = readl(base + DWMMC_BMOD);
	} while (ret & BMOD_SWRESET);

	fifo_size = readl(base + DWMMC_FIFOTH);
	fifo_size = ((fifo_size & FIFOTH_RWMARK_MASK) >> FIFOTH_RWMARK_SHIFT) + 1;
	data = FIFOTH_DMA_BURST_SIZE(0x2) | FIFOTH_RWMARK(fifo_size / 2 - 1) |
	    FIFOTH_TWMARK(fifo_size / 2);
	writel(data, base + DWMMC_FIFOTH);

	writel(0x0, base + DWMMC_IDINTEN);

	udelay(100);
	dw_set_clk(EMMC_BOOT_CLK_RATE);
	udelay(1000);
}

static int dw_send_cmd(emmc_cmd_t * cmd)
{
	uint32_t op, data, err_mask;
	uint64_t base;
	int32_t timeout;

	base = dw_params.reg_base;

	switch (cmd->cmd_idx) {
	case EMMC_CMD0:
		/* 1 << 15, sent init sequence before init cmd */
		op = CMD_SEND_INIT;
		break;
	case EMMC_CMD12:
		op = CMD_STOP_ABORT_CMD;
		break;
	case EMMC_CMD13:
		op = CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case EMMC_CMD8:
	case EMMC_CMD17:
	case EMMC_CMD18:
		op = CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case EMMC_CMD24:
	case EMMC_CMD25:
		op = CMD_WRITE | CMD_DATA_TRANS_EXPECT |
		    CMD_WAIT_PRVDATA_COMPLETE;
		break;
	default:
		op = 0;
		break;
	}

	op |= CMD_USE_HOLD_REG | CMD_START;
	switch (cmd->resp_type) {
	case 0:
		break;
	case EMMC_RESPONSE_R2:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC | CMD_RESP_LEN;
		break;
	case EMMC_RESPONSE_R3:
		op |= CMD_RESP_EXPECT;
		break;
	default:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC;
		break;
	}

	timeout = TIMEOUT;
	do {
		data = readl(base + DWMMC_STATUS);
		if (--timeout <= 0)
			panic("timeout %d\n", __LINE__);
	} while (data & STATUS_DATA_BUSY);

	/* clear raw int regs content */
	writel(~0, base + DWMMC_RINTSTS);
	/* cmd args to the card */
	writel(cmd->cmd_arg, base + DWMMC_CMDARG);
	/* 5:0, cmd idx */
	writel(op | cmd->cmd_idx, base + DWMMC_CMD);

	err_mask = INT_EBE | INT_HLE | INT_RTO | INT_RCRC | INT_RE |
	    INT_DCRC | INT_DRT | INT_SBE;
	timeout = TIMEOUT;
	do {
		data = readl(base + DWMMC_RINTSTS);

		if (data & err_mask) {
			/* Nothing to do */
		}

		if (data & INT_DTO)
			break;
		if (--timeout == 0) {
			printf("%s, RINTSTS:0x%x\n", __func__, data);
			panic("%s timeout %d\n", __FILE__, __LINE__);
		}
	} while (!(data & INT_CMD_DONE));

	if (op & CMD_RESP_EXPECT) {
		cmd->resp_data[0] = readl(base + DWMMC_RESP0);
		if (op & CMD_RESP_LEN) {
			cmd->resp_data[1] = readl(base + DWMMC_RESP1);
			cmd->resp_data[2] = readl(base + DWMMC_RESP2);
			cmd->resp_data[3] = readl(base + DWMMC_RESP3);
		}
	}
	return 0;
}

static int dw_set_ios(int clk, int width)
{
	uint64_t base;

	base = dw_params.reg_base;
	switch (width) {
	case EMMC_BUS_WIDTH_1:
		writel(CTYPE_1BIT, base + DWMMC_CTYPE);
		break;
	case EMMC_BUS_WIDTH_4:
		writel(CTYPE_4BIT, base + DWMMC_CTYPE);
		break;
	case EMMC_BUS_WIDTH_8:
		writel(CTYPE_8BIT, base + DWMMC_CTYPE);
		break;
	default:
		break;
	}
	dw_set_clk(clk);
	return 0;
}

static int dw_prepare(uint64_t lba, uint64_t buf, unsigned int size)
{
	uintptr_t base = dw_params.reg_base;
	uint32_t timeout = 1000;
	uint32_t ctrl;

	writel(EMMC_BLOCK_SIZE, base + DWMMC_BLKSIZ);
	writel(size, base + DWMMC_BYTCNT);

	ctrl = readl(base + DWMMC_CTRL);
	writel(ctrl | CTRL_FIFO_RESET, base + DWMMC_CTRL);

	while (timeout--) {
		ctrl = readl(base + DWMMC_CTRL);
		if (!(ctrl & CTRL_RESET_ALL))
			return 0;
	}

	return -1;
}

static int dw_fifo_ready(u32 bit, u32 *len)
{
	uintptr_t base = dw_params.reg_base;
	u32 timeout = 20000;

	*len = readl(base + DWMMC_STATUS);
	while (--timeout && (*len & bit)) {
		udelay(200);
		*len = readl(base + DWMMC_STATUS);
	}

	if (!timeout) {
		printf("%s: FIFO underflow timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int dw_read(uint64_t lba, uint64_t buf, unsigned int size)
{
	uintptr_t base = dw_params.reg_base;
	uint32_t mask, blocks, word_num, len;
	uint32_t i;
	uint32_t *pbuf = (uint32_t *) buf;

	blocks = (size + EMMC_BLOCK_SIZE - 1) / EMMC_BLOCK_SIZE;
	word_num = EMMC_BLOCK_SIZE * blocks / 4;

	for (;;) {
		mask = readl(base + DWMMC_RINTSTS);

		len = 0;
		if (mask & (INT_RXDR | INT_DTO)) {
			while (word_num) {
				len = readl(base + DWMMC_STATUS);
				len = (len >> STATUS_FIFO_SHIFT) & STATUS_FIFO_MASK;
				len = MIN(word_num, len);
				for (i = 0; i < len; i++) {
					*pbuf++ = readl(base + DWMMC_DATA);
				}
				word_num = word_num > len ? (word_num - len) : 0;
			}

			writel(INT_RXDR, base + DWMMC_RINTSTS);
		}

		/* Data arrived correctly. */
		if (mask & INT_DTO) {
			break;
		}
	}

	writel(mask, base + DWMMC_RINTSTS);

	return 0;
}

static int dw_write(uint64_t lba, const uint64_t buf, unsigned int size)
{
	uintptr_t base = dw_params.reg_base;
	uint32_t mask, blocks, word_num, len;
	uint32_t i;
	uint32_t *pbuf = (uint32_t *)buf;
	uint32_t fifo_depth;
	int ret = 0;

	fifo_depth = readl(base + DWMMC_FIFOTH);
	fifo_depth = (((fifo_depth & FIFOTH_RWMARK_MASK) >>
		FIFOTH_RWMARK_SHIFT) + 1) * 2;

	blocks = (size + EMMC_BLOCK_SIZE - 1) / EMMC_BLOCK_SIZE;
	word_num = EMMC_BLOCK_SIZE * blocks / 4;

	for (;;) {
		mask = readl(base + DWMMC_RINTSTS);

		if (mask & INT_TXDR) {
			while (word_num) {
				ret = dw_fifo_ready(STATUS_FIFO_FULL, &len);
				if (ret < 0) {
					break;
				}

				len = fifo_depth - ((len >> STATUS_FIFO_SHIFT) & STATUS_FIFO_MASK);
				len = min(word_num, len);

				for (i = 0; i < len; i++) {
					writel(*pbuf++, base + DWMMC_DATA);
				}

				word_num = word_num > len ? (word_num - len) : 0;
			}
			writel(INT_TXDR, base + DWMMC_RINTSTS);
		}

		/* Data arrived correctly. */
		if (mask & INT_DTO) {
			ret = 0;
			break;
		}
	}

	writel(mask, base + DWMMC_RINTSTS);

	return ret;
}

#if 0
static int dw_write(uint64_t lba, uint64_t buf, unsigned int size)
{
#ifdef HOBOT_DW_MMC_DMA_MODE
	return 0;
#else
	uint64_t base = dw_params.reg_base;
	uint32_t mask, blocks, word_num, len;
	uint32_t i;
	uint32_t *pbuf = (uint32_t *) buf;
	uint32_t fifo_depth = 0x100;

	blocks = (size + EMMC_BLOCK_SIZE - 1) / EMMC_BLOCK_SIZE;
	word_num = EMMC_BLOCK_SIZE * blocks / 4;

	for (;;) {
		mask = readl((void *)(base + DWMMC_RINTSTS));

		len = 0;
		if (mask & (INT_TXDR | INT_DTO)) {
			while (word_num) {
				len = readl((void *)(base + DWMMC_STATUS));
				len = fifo_depth - ((len >> STATUS_FIFO_SHIFT) &
				    STATUS_FIFO_MASK);
				len = MIN(word_num, len);
				for (i = 0; i < len; i++) {
					writel(*pbuf++, (void*)(base + DWMMC_DATA));
				}
				word_num =
				    word_num > len ? (word_num - len) : 0;
			}

			writel(INT_TXDR, (void *)(base + DWMMC_RINTSTS));
		}

		/* Data arrived correctly. */
		if (mask & INT_DTO) {
			break;
		}
	}

	writel(mask, (void *)(base + DWMMC_RINTSTS));

	return 0;
#endif /* HOBOT_DW_MMC_DMA_MODE */
}
#endif

emmc_ops_t *config_dw_mmc_ops(dw_mmc_params_t * params)
{
	memcpy(&dw_params, params, sizeof(dw_mmc_params_t));
	return &dw_mmc_ops;
}
#endif /* HB_MMC_BOOT */
