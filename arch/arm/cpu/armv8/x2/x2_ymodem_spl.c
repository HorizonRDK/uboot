
#include <asm/arch/x2_dev.h>
#include <xyzModem.h>

#include "x2_ymodem_spl.h"

#ifdef CONFIG_X2_YMODEM_BOOT

#define BUF_SIZE	1024

static int x2_ymodem_getc(void) {
	if (tstc())
		return (getc());
	return -1;
}

static unsigned int x2_ymodem_read(int lba, uint64_t buf, size_t size)
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
	xyzModem_stream_terminate(false, &x2_ymodem_getc);

	printf("Loaded %d bytes\n", total_size);

	return total_size;
}

void spl_x2_ymodem_init(void)
{
	g_dev_ops.proc_start = NULL;
	g_dev_ops.pre_read = NULL;
	g_dev_ops.read = x2_ymodem_read;
	g_dev_ops.post_read = NULL;
	g_dev_ops.proc_end = NULL;

	return;
}

#endif /* CONFIG_X2_YMODEM_BOOT */
