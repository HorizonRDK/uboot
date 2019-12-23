
#include <asm/io.h>

#include <asm/arch/hb_dev.h>

#if defined(CONFIG_HB_AP_BOOT) && defined(CONFIG_TARGET_HB)
#include <asm/arch/hb_share.h>

extern unsigned int ap_boot_type;

static void ap_start(void)
{
	/* It means that we can receive firmwares. */
	writel(DDRT_DW_RDY_BIT, HB_SHARE_DDRT_CTRL);

	return;
}

static void ap_stop(struct hb_info_hdr *pinfo)
{
	writel(DDRT_MEM_RDY_BIT, HB_SHARE_DDRT_CTRL);

	while ((readl(HB_SHARE_DDRT_CTRL) & DDRT_MEM_RDY_BIT));

	ap_boot_type = readl(HB_SHARE_DDRT_BOOT_TYPE);

	return;
}

static unsigned int ap_read_blk(uint64_t lba, uint64_t buf, size_t size)
{
	unsigned int temp;

	while (!(temp = readl(HB_SHARE_DDRT_CTRL) & DDRT_WR_RDY_BIT));

	return readl(HB_SHARE_DDRT_FW_SIZE);
}

static int ap_post_read(unsigned int flag)
{
	writel(flag, HB_SHARE_DDRT_CTRL);

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
#endif /* CONFIG_HB_AP_BOOT && CONFIG_TARGET_HB */

