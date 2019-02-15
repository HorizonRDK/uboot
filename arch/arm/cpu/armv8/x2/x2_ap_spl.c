
#include <asm/io.h>

#include <asm/arch/x2_dev.h>
#include <asm/arch/x2_share.h>

static unsigned int ap_read_blk(int lba, uint64_t buf, size_t size)
{
	unsigned int temp;

	while (!(temp = readl(X2_SHARE_DDRT_CTRL) & DDRT_WR_RDY_BIT));

	return readl(X2_SHARE_DDRT_FW_SIZE);
}

static int ap_post_read(unsigned int flag)
{
	writel(flag, X2_SHARE_DDRT_CTRL);

	return 0;
}

void spl_ap_init(void)
{
	g_dev_ops.pre_read = NULL;
	g_dev_ops.read = ap_read_blk;
	g_dev_ops.post_read = ap_post_read;

	return;
}
