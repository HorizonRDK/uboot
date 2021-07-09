/*
 *   Copyright 2021 Horizon Robotics, Inc.
 */
#define ISB	asm volatile ("isb sy" : : : "memory")
#define DSB	asm volatile ("dsb sy" : : : "memory")
#define DMB	asm volatile ("dmb sy" : : : "memory")
#define isb()	ISB
#define dsb()	DSB
#define dmb()	DMB
/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_putl(v, a)		(*(volatile unsigned int *)(a) = (v))
/*
 * TODO: The kernel offers some more advanced versions of barriers, it might
 * have some advantages to use them instead of the simple one here.
 */
#define mb()		dsb()
#define __iormb()	dmb()
#define __iowmb()	dmb()

#define writel(v, c)	({ unsigned int __v = v; __iowmb(); __arch_putl(__v, c); __v; })
#define readl(c)	({ unsigned int __v = __arch_getl(c); __iormb(); __v; })

static void set_ddrc_qos(unsigned int write_qos, unsigned int read_qos)
{
#define REG_DDR_PORT_READ_QOS_CTRL 0xA2D10000
#define REG_DDR_PORT_WRITE_QOS_CTRL 0xA2D10004
	// [0]: ACE
	// [1]: NOC
	// [2]: CNN0
	// [3]: CNN1
	// [4]: VIO0
	// [5]: VSP
	// [6]: VIO1
	// [7]: peri

	writel(read_qos, REG_DDR_PORT_READ_QOS_CTRL);
	writel(write_qos, REG_DDR_PORT_WRITE_QOS_CTRL);
}

void do_update_qos(unsigned int write_qos, unsigned int read_qos)
{
	unsigned int port_num = 0;
	unsigned long reg_addr = 0;

	//a. diable axi port n
	for(port_num = 0; port_num < 8; port_num++) {
		reg_addr = 0xA2D00490 + 0xB0 * port_num;
		writel(0, reg_addr);
	}

	//b. polling axi port n is in idle state
	do {
	}while(0x0 != (readl(0xA2D003FC)));

	//c. Set QoS
	set_ddrc_qos(write_qos, read_qos);

	//d. enable axi port n
	for(port_num = 0; port_num < 8; port_num++) {
		reg_addr = 0xA2D00490 + 0xB0 * port_num;
		writel(1, reg_addr);
	}
}
