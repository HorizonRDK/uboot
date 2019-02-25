
#include <asm/io.h>

#include <asm/arch/x2_dev.h>

#if defined(CONFIG_X2_AP_BOOT) && defined(CONFIG_TARGET_X2)
#include <asm/arch/x2_share.h>

static void ap_start(void)
{
	/* It means that we can receive firmwares. */
	writel(DDRT_DW_RDY_BIT, X2_SHARE_DDRT_CTRL);

	return;
}

static void ap_stop(void)
{
	writel(DDRT_MEM_RDY_BIT, X2_SHARE_DDRT_CTRL);

	while ((readl(X2_SHARE_DDRT_CTRL) & DDRT_MEM_RDY_BIT));

	return;
}

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
	g_dev_ops.proc_start = ap_start;
	g_dev_ops.pre_read = NULL;
	g_dev_ops.read = ap_read_blk;
	g_dev_ops.post_read = ap_post_read;
	g_dev_ops.proc_end = ap_stop;

	return;
}
#endif /* CONFIG_X2_AP_BOOT && CONFIG_TARGET_X2 */

