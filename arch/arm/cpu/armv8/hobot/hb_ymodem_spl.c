#ifdef CONFIG_HB_YMODEM_BOOT
#include <asm/io.h>
#include <asm/arch/hb_dev.h>
#include <xyzModem.h>

#include "hb_ymodem_spl.h"

#define BUF_SIZE	1024

static int hb_ymodem_getc(void) {
	if (tstc())
		return (getc());
	return -1;
}

static unsigned int hb_ymodem_read(uint64_t lba, uint64_t buf, size_t size)
{
	int total_size = 0;
	int err;
	int res;
	int ret;
	connection_info_t info;
	char *pbuf = (char *)buf;

	info.mode = xyzModem_ymodem;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("spl: ymodem err - %s\n", xyzModem_error(err));
		return ret;
	}

	while ((res = xyzModem_stream_read(pbuf, BUF_SIZE, &err)) > 0) {
			total_size += res;
			pbuf += res;
	}

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &hb_ymodem_getc);

	printf("Loaded %d bytes\n", total_size);

	return total_size;
}

void hb_ymodem_test(struct hb_info_hdr *pinfo)
{
#if 0
	unsigned int pattern = 0xdeadbeaf;

	for (int i = 0; i < 100; i++) {
		printf("%d, 0x%x\n", i, readl(0x20000000));
	}

	for (int i = 0; i < 100; i++) {
		writel(pattern, 0x20000000);
		printf("%d, 0x%x\n", i, readl(0x20000000));
	}
#endif /* #if 0 */

	return;
}

void spl_hb_ymodem_init(void)
{
	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = NULL;
	g_dev_ops.read = hb_ymodem_read;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = hb_ymodem_test;

	return;
}

#endif /* CONFIG_HB_YMODEM_BOOT */
