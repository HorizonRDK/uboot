#ifndef __ASM_ARCH_X2_DEV_H__
#define __ASM_ARCH_X2_DEV_H__

#include <common.h>
#include <hb_info.h>

struct hb_dev_ops {
	void (*proc_start)(void);
	void (*pre_read)(struct hb_info_hdr *pinfo,
		int tr_num, int tr_type,
		unsigned int *pload_addr, unsigned int *pload_size);

	uint32_t (*read)(uint64_t lba, uint64_t buf, size_t size);
	uint32_t (*write)(uint64_t lba, const uintptr_t buf, size_t size);
	int (*erase)(uint64_t lba, size_t size);

	int (*post_read)(unsigned int flag);
	void (*proc_end)(struct hb_info_hdr *pinfo);
};

extern struct hb_dev_ops g_dev_ops;

#endif /* __ASM_ARCH_X2_DEV_H__ */
