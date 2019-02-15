#ifndef __ASM_ARCH_X2_DEV_H__
#define __ASM_ARCH_X2_DEV_H__

#include <common.h>
#include "../../../cpu/armv8/x2/x2_info.h"

struct x2_dev_ops {
	unsigned int (*pre_read)(struct x2_info_hdr *pinfo,
		int tr_num, unsigned int offset);

	unsigned int (*read)(int lba, uint64_t buf, size_t size);

	int (*post_read)(unsigned int flag);
};

extern struct x2_dev_ops g_dev_ops;

#endif /* __ASM_ARCH_X2_DEV_H__ */
