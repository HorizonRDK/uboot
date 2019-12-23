#ifndef __ASM_ARCH_HB_DDR_H__
#define __ASM_ARCH_HB_DDR_H__

#include <asm/io.h>
#include <asm/types.h>
#include <asm/arch/hb_reg.h>

#include <hb_info.h>
//#include <asm/arch/hb_mmc_spl.h>

/* ddr controller registers */
#define DDRC_MSTR             (DDRC_BASE_ADDR + 0x00)
#define DDRC_STAT             (DDRC_BASE_ADDR + 0x04)
#define DDRC_MSTR1            (DDRC_BASE_ADDR + 0x08)
#define DDRC_MRCTRL0          (DDRC_BASE_ADDR + 0x10)
#define DDRC_MRCTRL1          (DDRC_BASE_ADDR + 0x14)
#define DDRC_MRSTAT           (DDRC_BASE_ADDR + 0x18)
#define DDRC_MRCTRL2          (DDRC_BASE_ADDR + 0x1c)
#define DDRC_DERATEEN         (DDRC_BASE_ADDR + 0x20)
#define DDRC_DERATEINT        (DDRC_BASE_ADDR + 0x24)
#define DDRC_DERATECTL        (DDRC_BASE_ADDR + 0x2c)
#define DDRC_PWRCTL           (DDRC_BASE_ADDR + 0x30)
#define DDRC_PWRTMG           (DDRC_BASE_ADDR + 0x34)
#define DDRC_HWLPCTL          (DDRC_BASE_ADDR + 0x38)
#define DDRC_HWFFCCTL         (DDRC_BASE_ADDR + 0x3c)
#define DDRC_HWFFCSTAT        (DDRC_BASE_ADDR + 0x40)
#define DDRC_RFSHCTL0         (DDRC_BASE_ADDR + 0x50)
#define DDRC_RFSHCTL1         (DDRC_BASE_ADDR + 0x54)
#define DDRC_RFSHCTL2         (DDRC_BASE_ADDR + 0x58)
#define DDRC_RFSHCTL3         (DDRC_BASE_ADDR + 0x60)
#define DDRC_RFSHTMG          (DDRC_BASE_ADDR + 0x64)
#define DDRC_RFSHTMG1         (DDRC_BASE_ADDR + 0x68)
#define DDRC_ECCCFG0          (DDRC_BASE_ADDR + 0x70)
#define DDRC_ECCCFG1          (DDRC_BASE_ADDR + 0x74)
#define DDRC_ECCSTAT          (DDRC_BASE_ADDR + 0x78)
#define DDRC_ECCCLR           (DDRC_BASE_ADDR + 0x7c)
#define DDRC_ECCERRCNT        (DDRC_BASE_ADDR + 0x80)
#define DDRC_ECCCADDR0        (DDRC_BASE_ADDR + 0x84)
#define DDRC_ECCCADDR1        (DDRC_BASE_ADDR + 0x88)
#define DDRC_ECCCSYN0         (DDRC_BASE_ADDR + 0x8c)
#define DDRC_ECCCSYN1         (DDRC_BASE_ADDR + 0x90)
#define DDRC_ECCCSYN2         (DDRC_BASE_ADDR + 0x94)
#define DDRC_ECCBITMASK0      (DDRC_BASE_ADDR + 0x98)
#define DDRC_ECCBITMASK1      (DDRC_BASE_ADDR + 0x9c)
#define DDRC_ECCBITMASK2      (DDRC_BASE_ADDR + 0xa0)
#define DDRC_ECCUADDR0        (DDRC_BASE_ADDR + 0xa4)
#define DDRC_ECCUADDR1        (DDRC_BASE_ADDR + 0xa8)
#define DDRC_ECCUSYN0         (DDRC_BASE_ADDR + 0xac)
#define DDRC_ECCUSYN1         (DDRC_BASE_ADDR + 0xb0)
#define DDRC_ECCUSYN2         (DDRC_BASE_ADDR + 0xb4)
#define DDRC_ECCPOISONADDR0   (DDRC_BASE_ADDR + 0xb8)
#define DDRC_ECCPOISONADDR1   (DDRC_BASE_ADDR + 0xbc)
#define DDRC_CRCPARCTL0       (DDRC_BASE_ADDR + 0xc0)
#define DDRC_CRCPARCTL1       (DDRC_BASE_ADDR + 0xc4)
#define DDRC_CRCPARCTL2       (DDRC_BASE_ADDR + 0xc8)
#define DDRC_CRCPARSTAT       (DDRC_BASE_ADDR + 0xcc)
#define DDRC_INIT0            (DDRC_BASE_ADDR + 0xd0)
#define DDRC_INIT1            (DDRC_BASE_ADDR + 0xd4)
#define DDRC_INIT2            (DDRC_BASE_ADDR + 0xd8)
#define DDRC_INIT3            (DDRC_BASE_ADDR + 0xdc)
#define DDRC_INIT4            (DDRC_BASE_ADDR + 0xe0)
#define DDRC_INIT5            (DDRC_BASE_ADDR + 0xe4)
#define DDRC_INIT6            (DDRC_BASE_ADDR + 0xe8)
#define DDRC_INIT7            (DDRC_BASE_ADDR + 0xec)
#define DDRC_DIMMCTL          (DDRC_BASE_ADDR + 0xf0)
#define DDRC_RANKCTL          (DDRC_BASE_ADDR + 0xf4)
#define DDRC_DRAMTMG0         (DDRC_BASE_ADDR + 0x100)
#define DDRC_DRAMTMG1         (DDRC_BASE_ADDR + 0x104)
#define DDRC_DRAMTMG2         (DDRC_BASE_ADDR + 0x108)
#define DDRC_DRAMTMG3         (DDRC_BASE_ADDR + 0x10c)
#define DDRC_DRAMTMG4         (DDRC_BASE_ADDR + 0x110)
#define DDRC_DRAMTMG5         (DDRC_BASE_ADDR + 0x114)
#define DDRC_DRAMTMG6         (DDRC_BASE_ADDR + 0x118)
#define DDRC_DRAMTMG7         (DDRC_BASE_ADDR + 0x11c)
#define DDRC_DRAMTMG8         (DDRC_BASE_ADDR + 0x120)
#define DDRC_DRAMTMG9         (DDRC_BASE_ADDR + 0x124)
#define DDRC_DRAMTMG10        (DDRC_BASE_ADDR + 0x128)
#define DDRC_DRAMTMG11        (DDRC_BASE_ADDR + 0x12c)
#define DDRC_DRAMTMG12        (DDRC_BASE_ADDR + 0x130)
#define DDRC_DRAMTMG13        (DDRC_BASE_ADDR + 0x134)
#define DDRC_DRAMTMG14        (DDRC_BASE_ADDR + 0x138)
#define DDRC_DRAMTMG15        (DDRC_BASE_ADDR + 0x13C)
#define DDRC_DRAMTMG16        (DDRC_BASE_ADDR + 0x140)
#define DDRC_DRAMTMG17        (DDRC_BASE_ADDR + 0x144)
#define DDRC_ZQCTL0           (DDRC_BASE_ADDR + 0x180)
#define DDRC_ZQCTL1           (DDRC_BASE_ADDR + 0x184)
#define DDRC_ZQCTL2           (DDRC_BASE_ADDR + 0x188)
#define DDRC_ZQSTAT           (DDRC_BASE_ADDR + 0x18c)
#define DDRC_DFITMG0          (DDRC_BASE_ADDR + 0x190)
#define DDRC_DFITMG1          (DDRC_BASE_ADDR + 0x194)
#define DDRC_DFILPCFG0        (DDRC_BASE_ADDR + 0x198)
#define DDRC_DFILPCFG1        (DDRC_BASE_ADDR + 0x19c)
#define DDRC_DFIUPD0          (DDRC_BASE_ADDR + 0x1a0)
#define DDRC_DFIUPD1          (DDRC_BASE_ADDR + 0x1a4)
#define DDRC_DFIUPD2          (DDRC_BASE_ADDR + 0x1a8)
#define DDRC_DFIMISC          (DDRC_BASE_ADDR + 0x1b0)
#define DDRC_DFITMG2          (DDRC_BASE_ADDR + 0x1b4)
#define DDRC_DFITMG3          (DDRC_BASE_ADDR + 0x1b8)
#define DDRC_DFISTAT          (DDRC_BASE_ADDR + 0x1bc)
#define DDRC_DBICTL           (DDRC_BASE_ADDR + 0x1c0)
#define DDRC_DFIPHYMSTR       (DDRC_BASE_ADDR + 0x1c4)
#define DDRC_TRAINCTL0        (DDRC_BASE_ADDR + 0x1d0)
#define DDRC_TRAINCTL1        (DDRC_BASE_ADDR + 0x1d4)
#define DDRC_TRAINCTL2        (DDRC_BASE_ADDR + 0x1d8)
#define DDRC_TRAINSTAT        (DDRC_BASE_ADDR + 0x1dc)
#define DDRC_ADDRMAP0         (DDRC_BASE_ADDR + 0x200)
#define DDRC_ADDRMAP1         (DDRC_BASE_ADDR + 0x204)
#define DDRC_ADDRMAP2         (DDRC_BASE_ADDR + 0x208)
#define DDRC_ADDRMAP3         (DDRC_BASE_ADDR + 0x20c)
#define DDRC_ADDRMAP4         (DDRC_BASE_ADDR + 0x210)
#define DDRC_ADDRMAP5         (DDRC_BASE_ADDR + 0x214)
#define DDRC_ADDRMAP6         (DDRC_BASE_ADDR + 0x218)
#define DDRC_ADDRMAP7         (DDRC_BASE_ADDR + 0x21c)
#define DDRC_ADDRMAP8         (DDRC_BASE_ADDR + 0x220)
#define DDRC_ADDRMAP9         (DDRC_BASE_ADDR + 0x224)
#define DDRC_ADDRMAP10        (DDRC_BASE_ADDR + 0x228)
#define DDRC_ADDRMAP11        (DDRC_BASE_ADDR + 0x22c)
#define DDRC_ODTCFG           (DDRC_BASE_ADDR + 0x240)
#define DDRC_ODTMAP           (DDRC_BASE_ADDR + 0x244)
#define DDRC_SCHED            (DDRC_BASE_ADDR + 0x250)
#define DDRC_SCHED1           (DDRC_BASE_ADDR + 0x254)
#define DDRC_PERFHPR1         (DDRC_BASE_ADDR + 0x25c)
#define DDRC_PERFLPR1         (DDRC_BASE_ADDR + 0x264)
#define DDRC_PERFWR1          (DDRC_BASE_ADDR + 0x26c)
#define DDRC_PERFVPR1         (DDRC_BASE_ADDR + 0x274)
#define DDRC_PERFVPW1         (DDRC_BASE_ADDR + 0x278)
#define DDRC_DQMAP0           (DDRC_BASE_ADDR + 0x280)
#define DDRC_DQMAP1           (DDRC_BASE_ADDR + 0x284)
#define DDRC_DQMAP2           (DDRC_BASE_ADDR + 0x288)
#define DDRC_DQMAP3           (DDRC_BASE_ADDR + 0x28c)
#define DDRC_DQMAP4           (DDRC_BASE_ADDR + 0x290)
#define DDRC_DQMAP5           (DDRC_BASE_ADDR + 0x294)
#define DDRC_DBG0             (DDRC_BASE_ADDR + 0x300)
#define DDRC_DBG1             (DDRC_BASE_ADDR + 0x304)
#define DDRC_DBGCAM           (DDRC_BASE_ADDR + 0x308)
#define DDRC_DBGCMD           (DDRC_BASE_ADDR + 0x30c)
#define DDRC_DBGSTAT          (DDRC_BASE_ADDR + 0x310)
#define DDRC_SWCTL            (DDRC_BASE_ADDR + 0x320)
#define DDRC_SWSTAT           (DDRC_BASE_ADDR + 0x324)
#define DDRC_OCPARCFG0        (DDRC_BASE_ADDR + 0x330)
#define DDRC_OCPARCFG1        (DDRC_BASE_ADDR + 0x334)
#define DDRC_OCPARCFG2        (DDRC_BASE_ADDR + 0x338)
#define DDRC_OCPARCFG3        (DDRC_BASE_ADDR + 0x33c)
#define DDRC_OCPARSTAT0       (DDRC_BASE_ADDR + 0x340)
#define DDRC_OCPARSTAT1       (DDRC_BASE_ADDR + 0x344)
#define DDRC_OCPARWLOG0       (DDRC_BASE_ADDR + 0x348)
#define DDRC_OCPARWLOG1       (DDRC_BASE_ADDR + 0x34c)
#define DDRC_OCPARWLOG2       (DDRC_BASE_ADDR + 0x350)
#define DDRC_OCPARAWLOG0      (DDRC_BASE_ADDR + 0x354)
#define DDRC_OCPARAWLOG1      (DDRC_BASE_ADDR + 0x358)
#define DDRC_OCPARRLOG0       (DDRC_BASE_ADDR + 0x35c)
#define DDRC_OCPARRLOG1       (DDRC_BASE_ADDR + 0x360)
#define DDRC_OCPARARLOG0      (DDRC_BASE_ADDR + 0x364)
#define DDRC_OCPARARLOG1      (DDRC_BASE_ADDR + 0x368)
#define DDRC_POISONCFG        (DDRC_BASE_ADDR + 0x36C)
#define DDRC_POISONSTAT       (DDRC_BASE_ADDR + 0x370)

#define DDRC_PSTAT            (DDRC_BASE_ADDR + 0x3fc)
#define DDRC_PCCFG            (DDRC_BASE_ADDR + 0x400)
#define DDRC_PCFGR_0          (DDRC_BASE_ADDR + 0x404)
#define DDRC_PCFGR_1          (DDRC_BASE_ADDR + 1 * 0xb0 + 0x404)
#define DDRC_PCFGR_2          (DDRC_BASE_ADDR + 2 * 0xb0 + 0x404)
#define DDRC_PCFGR_3          (DDRC_BASE_ADDR + 3 * 0xb0 + 0x404)
#define DDRC_PCFGR_4          (DDRC_BASE_ADDR + 4 * 0xb0 + 0x404)
#define DDRC_PCFGR_5          (DDRC_BASE_ADDR + 5 * 0xb0 + 0x404)

#define DDRC_PCFGW_0          (DDRC_BASE_ADDR + 0x408)
#define DDRC_PCFGW_1          (DDRC_BASE_ADDR + 1 * 0xb0 + 0x408)
#define DDRC_PCFGW_2          (DDRC_BASE_ADDR + 2 * 0xb0 + 0x408)
#define DDRC_PCFGW_3          (DDRC_BASE_ADDR + 3 * 0xb0 + 0x408)
#define DDRC_PCFGW_4          (DDRC_BASE_ADDR + 4 * 0xb0 + 0x408)
#define DDRC_PCFGW_5          (DDRC_BASE_ADDR + 5 * 0xb0 + 0x408)

#define DDRC_PCFGC_0          (DDRC_BASE_ADDR + 0x40c)
#define DDRC_PCFGIDMASKCH     (DDRC_BASE_ADDR + 0x410)
#define DDRC_PCFGIDVALUECH    (DDRC_BASE_ADDR + 0x414)
#define DDRC_PCTRL_0          (DDRC_BASE_ADDR + 0x490)
#define DDRC_PCTRL_1          (DDRC_BASE_ADDR + 0x490 + 1 * 0xb0)
#define DDRC_PCTRL_2          (DDRC_BASE_ADDR + 0x490 + 2 * 0xb0)
#define DDRC_PCTRL_3          (DDRC_BASE_ADDR + 0x490 + 3 * 0xb0)
#define DDRC_PCTRL_4          (DDRC_BASE_ADDR + 0x490 + 4 * 0xb0)
#define DDRC_PCTRL_5          (DDRC_BASE_ADDR + 0x490 + 5 * 0xb0)

#define DDRC_PCFGQOS0_0       (DDRC_BASE_ADDR + 0x494)
#define DDRC_PCFGQOS0_1       (DDRC_BASE_ADDR + 0x494 + 1 * 0xb0)
#define DDRC_PCFGQOS0_2       (DDRC_BASE_ADDR + 0x494 + 2 * 0xb0)
#define DDRC_PCFGQOS0_3       (DDRC_BASE_ADDR + 0x494 + 3 * 0xb0)
#define DDRC_PCFGQOS0_4       (DDRC_BASE_ADDR + 0x494 + 4 * 0xb0)
#define DDRC_PCFGQOS0_5       (DDRC_BASE_ADDR + 0x494 + 5 * 0xb0)

#define DDRC_PCFGQOS1_0       (DDRC_BASE_ADDR + 0x498)
#define DDRC_PCFGQOS1_1       (DDRC_BASE_ADDR + 0x498 + 1 * 0xb0)
#define DDRC_PCFGQOS1_2       (DDRC_BASE_ADDR + 0x498 + 2 * 0xb0)
#define DDRC_PCFGQOS1_3       (DDRC_BASE_ADDR + 0x498 + 3 * 0xb0)
#define DDRC_PCFGQOS1_4       (DDRC_BASE_ADDR + 0x498 + 4 * 0xb0)
#define DDRC_PCFGQOS1_5       (DDRC_BASE_ADDR + 0x498 + 5 * 0xb0)

#define DDRC_PCFGWQOS0_0      (DDRC_BASE_ADDR + 0x49c)
#define DDRC_PCFGWQOS0_1      (DDRC_BASE_ADDR + 0x49c + 1 * 0xb0)
#define DDRC_PCFGWQOS0_2      (DDRC_BASE_ADDR + 0x49c + 2 * 0xb0)
#define DDRC_PCFGWQOS0_3      (DDRC_BASE_ADDR + 0x49c + 3 * 0xb0)
#define DDRC_PCFGWQOS0_4      (DDRC_BASE_ADDR + 0x49c + 4 * 0xb0)
#define DDRC_PCFGWQOS0_5      (DDRC_BASE_ADDR + 0x49c + 5 * 0xb0)

#define DDRC_PCFGWQOS1_0      (DDRC_BASE_ADDR + 0x4a0)
#define DDRC_PCFGWQOS1_1      (DDRC_BASE_ADDR + 0x4a0 + 1 * 0xb0)
#define DDRC_PCFGWQOS1_2      (DDRC_BASE_ADDR + 0x4a0 + 2 * 0xb0)
#define DDRC_PCFGWQOS1_3      (DDRC_BASE_ADDR + 0x4a0 + 3 * 0xb0)
#define DDRC_PCFGWQOS1_4      (DDRC_BASE_ADDR + 0x4a0 + 4 * 0xb0)
#define DDRC_PCFGWQOS1_5      (DDRC_BASE_ADDR + 0x4a0 + 5 * 0xb0)

#define DDRC_SARBASE0         (DDRC_BASE_ADDR + 0xf04)
#define DDRC_SARSIZE0         (DDRC_BASE_ADDR + 0xf08)
#define DDRC_SBRCTL           (DDRC_BASE_ADDR + 0xf24)
#define DDRC_SBRSTAT          (DDRC_BASE_ADDR + 0xf28)
#define DDRC_SBRWDATA0        (DDRC_BASE_ADDR + 0xf2c)
#define DDRC_SBRWDATA1        (DDRC_BASE_ADDR + 0xf30)
#define DDRC_PDCH             (DDRC_BASE_ADDR + 0xf34)

#define DDRC_FREQ1_DERATEEN         (DDRC_BASE_ADDR + 0x2020)
#define DDRC_FREQ1_DERATEINT        (DDRC_BASE_ADDR + 0x2024)
#define DDRC_FREQ1_RFSHCTL0         (DDRC_BASE_ADDR + 0x2050)
#define DDRC_FREQ1_RFSHTMG          (DDRC_BASE_ADDR + 0x2064)
#define DDRC_FREQ1_INIT3            (DDRC_BASE_ADDR + 0x20dc)
#define DDRC_FREQ1_INIT4            (DDRC_BASE_ADDR + 0x20e0)
#define DDRC_FREQ1_INIT6            (DDRC_BASE_ADDR + 0x20e8)
#define DDRC_FREQ1_INIT7            (DDRC_BASE_ADDR + 0x20ec)
#define DDRC_FREQ1_DRAMTMG0         (DDRC_BASE_ADDR + 0x2100)
#define DDRC_FREQ1_DRAMTMG1         (DDRC_BASE_ADDR + 0x2104)
#define DDRC_FREQ1_DRAMTMG2         (DDRC_BASE_ADDR + 0x2108)
#define DDRC_FREQ1_DRAMTMG3         (DDRC_BASE_ADDR + 0x210c)
#define DDRC_FREQ1_DRAMTMG4         (DDRC_BASE_ADDR + 0x2110)
#define DDRC_FREQ1_DRAMTMG5         (DDRC_BASE_ADDR + 0x2114)
#define DDRC_FREQ1_DRAMTMG6         (DDRC_BASE_ADDR + 0x2118)
#define DDRC_FREQ1_DRAMTMG7         (DDRC_BASE_ADDR + 0x211c)
#define DDRC_FREQ1_DRAMTMG8         (DDRC_BASE_ADDR + 0x2120)
#define DDRC_FREQ1_DRAMTMG9         (DDRC_BASE_ADDR + 0x2124)
#define DDRC_FREQ1_DRAMTMG10        (DDRC_BASE_ADDR + 0x2128)
#define DDRC_FREQ1_DRAMTMG11        (DDRC_BASE_ADDR + 0x212c)
#define DDRC_FREQ1_DRAMTMG12        (DDRC_BASE_ADDR + 0x2130)
#define DDRC_FREQ1_DRAMTMG13        (DDRC_BASE_ADDR + 0x2134)
#define DDRC_FREQ1_DRAMTMG14        (DDRC_BASE_ADDR + 0x2138)
#define DDRC_FREQ1_DRAMTMG15        (DDRC_BASE_ADDR + 0x213C)
#define DDRC_FREQ1_DRAMTMG16        (DDRC_BASE_ADDR + 0x2140)
#define DDRC_FREQ1_DRAMTMG17        (DDRC_BASE_ADDR + 0x2144)
#define DDRC_FREQ1_ZQCTL0           (DDRC_BASE_ADDR + 0x2180)
#define DDRC_FREQ1_DFITMG0          (DDRC_BASE_ADDR + 0x2190)
#define DDRC_FREQ1_DFITMG1          (DDRC_BASE_ADDR + 0x2194)
#define DDRC_FREQ1_DFITMG2          (DDRC_BASE_ADDR + 0x21b4)
#define DDRC_FREQ1_DFITMG3          (DDRC_BASE_ADDR + 0x21b8)
#define DDRC_FREQ1_ODTCFG           (DDRC_BASE_ADDR + 0x2240)

#define DDRC_FREQ2_DERATEEN         (DDRC_BASE_ADDR + 0x3020)
#define DDRC_FREQ2_DERATEINT        (DDRC_BASE_ADDR + 0x3024)
#define DDRC_FREQ2_RFSHCTL0         (DDRC_BASE_ADDR + 0x3050)
#define DDRC_FREQ2_RFSHTMG          (DDRC_BASE_ADDR + 0x3064)
#define DDRC_FREQ2_INIT3            (DDRC_BASE_ADDR + 0x30dc)
#define DDRC_FREQ2_INIT4            (DDRC_BASE_ADDR + 0x30e0)
#define DDRC_FREQ2_INIT6            (DDRC_BASE_ADDR + 0x30e8)
#define DDRC_FREQ2_INIT7            (DDRC_BASE_ADDR + 0x30ec)
#define DDRC_FREQ2_DRAMTMG0         (DDRC_BASE_ADDR + 0x3100)
#define DDRC_FREQ2_DRAMTMG1         (DDRC_BASE_ADDR + 0x3104)
#define DDRC_FREQ2_DRAMTMG2         (DDRC_BASE_ADDR + 0x3108)
#define DDRC_FREQ2_DRAMTMG3         (DDRC_BASE_ADDR + 0x310c)
#define DDRC_FREQ2_DRAMTMG4         (DDRC_BASE_ADDR + 0x3110)
#define DDRC_FREQ2_DRAMTMG5         (DDRC_BASE_ADDR + 0x3114)
#define DDRC_FREQ2_DRAMTMG6         (DDRC_BASE_ADDR + 0x3118)
#define DDRC_FREQ2_DRAMTMG7         (DDRC_BASE_ADDR + 0x311c)
#define DDRC_FREQ2_DRAMTMG8         (DDRC_BASE_ADDR + 0x3120)
#define DDRC_FREQ2_DRAMTMG9         (DDRC_BASE_ADDR + 0x3124)
#define DDRC_FREQ2_DRAMTMG10        (DDRC_BASE_ADDR + 0x3128)
#define DDRC_FREQ2_DRAMTMG11        (DDRC_BASE_ADDR + 0x312c)
#define DDRC_FREQ2_DRAMTMG12        (DDRC_BASE_ADDR + 0x3130)
#define DDRC_FREQ2_DRAMTMG13        (DDRC_BASE_ADDR + 0x3134)
#define DDRC_FREQ2_DRAMTMG14        (DDRC_BASE_ADDR + 0x3138)
#define DDRC_FREQ2_DRAMTMG15        (DDRC_BASE_ADDR + 0x313C)
#define DDRC_FREQ2_DRAMTMG16        (DDRC_BASE_ADDR + 0x3140)
#define DDRC_FREQ2_DRAMTMG17        (DDRC_BASE_ADDR + 0x3144)
#define DDRC_FREQ2_ZQCTL0           (DDRC_BASE_ADDR + 0x3180)
#define DDRC_FREQ2_DFITMG0          (DDRC_BASE_ADDR + 0x3190)
#define DDRC_FREQ2_DFITMG1          (DDRC_BASE_ADDR + 0x3194)
#define DDRC_FREQ2_DFITMG2          (DDRC_BASE_ADDR + 0x31b4)
#define DDRC_FREQ2_DFITMG3          (DDRC_BASE_ADDR + 0x31b8)
#define DDRC_FREQ2_ODTCFG           (DDRC_BASE_ADDR + 0x3240)

#define DDRC_FREQ3_DERATEEN         (DDRC_BASE_ADDR + 0x4020)
#define DDRC_FREQ3_DERATEINT        (DDRC_BASE_ADDR + 0x4024)
#define DDRC_FREQ3_RFSHCTL0         (DDRC_BASE_ADDR + 0x4050)
#define DDRC_FREQ3_RFSHTMG          (DDRC_BASE_ADDR + 0x4064)
#define DDRC_FREQ3_INIT3            (DDRC_BASE_ADDR + 0x40dc)
#define DDRC_FREQ3_INIT4            (DDRC_BASE_ADDR + 0x40e0)
#define DDRC_FREQ3_INIT6            (DDRC_BASE_ADDR + 0x40e8)
#define DDRC_FREQ3_INIT7            (DDRC_BASE_ADDR + 0x40ec)
#define DDRC_FREQ3_DRAMTMG0         (DDRC_BASE_ADDR + 0x4100)
#define DDRC_FREQ3_DRAMTMG1         (DDRC_BASE_ADDR + 0x4104)
#define DDRC_FREQ3_DRAMTMG2         (DDRC_BASE_ADDR + 0x4108)
#define DDRC_FREQ3_DRAMTMG3         (DDRC_BASE_ADDR + 0x410c)
#define DDRC_FREQ3_DRAMTMG4         (DDRC_BASE_ADDR + 0x4110)
#define DDRC_FREQ3_DRAMTMG5         (DDRC_BASE_ADDR + 0x4114)
#define DDRC_FREQ3_DRAMTMG6         (DDRC_BASE_ADDR + 0x4118)
#define DDRC_FREQ3_DRAMTMG7         (DDRC_BASE_ADDR + 0x411c)
#define DDRC_FREQ3_DRAMTMG8         (DDRC_BASE_ADDR + 0x4120)
#define DDRC_FREQ3_DRAMTMG9         (DDRC_BASE_ADDR + 0x4124)
#define DDRC_FREQ3_DRAMTMG10        (DDRC_BASE_ADDR + 0x4128)
#define DDRC_FREQ3_DRAMTMG11        (DDRC_BASE_ADDR + 0x412c)
#define DDRC_FREQ3_DRAMTMG12        (DDRC_BASE_ADDR + 0x4130)
#define DDRC_FREQ3_DRAMTMG13        (DDRC_BASE_ADDR + 0x4134)
#define DDRC_FREQ3_DRAMTMG14        (DDRC_BASE_ADDR + 0x4138)
#define DDRC_FREQ3_DRAMTMG15        (DDRC_BASE_ADDR + 0x413C)
#define DDRC_FREQ3_DRAMTMG16        (DDRC_BASE_ADDR + 0x4140)

#define DDRC_FREQ3_ZQCTL0           (DDRC_BASE_ADDR + 0x4180)
#define DDRC_FREQ3_DFITMG0          (DDRC_BASE_ADDR + 0x4190)
#define DDRC_FREQ3_DFITMG1          (DDRC_BASE_ADDR + 0x4194)
#define DDRC_FREQ3_DFITMG2          (DDRC_BASE_ADDR + 0x41b4)
#define DDRC_FREQ3_DFITMG3          (DDRC_BASE_ADDR + 0x41b8)
#define DDRC_FREQ3_ODTCFG           (DDRC_BASE_ADDR + 0x4240)
#define DDRC_DFITMG0_SHADOW         (DDRC_BASE_ADDR + 0x2190)
#define DDRC_DFITMG1_SHADOW         (DDRC_BASE_ADDR + 0x2194)
#define DDRC_DFITMG2_SHADOW         (DDRC_BASE_ADDR + 0x21b4)
#define DDRC_DFITMG3_SHADOW         (DDRC_BASE_ADDR + 0x21b8)
#define DDRC_ODTCFG_SHADOW          (DDRC_BASE_ADDR + 0x2240)

/* ddr phy dfi register */
#define DDRC_PHY_DFI1_ENABLE		(DDRC_PERF_MON_ADDR + 0x08)

/* user data type */
enum fw_type {
	FW_1D_IMAGE,
	FW_2D_IMAGE,
};

struct dram_cfg_param {
	unsigned int reg;
	unsigned int val;
};

struct dram_fsp_msg {
	unsigned int drate;
	enum fw_type fw_type;
	struct dram_cfg_param *fsp_cfg;
	unsigned int fsp_cfg_num;
};

struct dram_timing_info {
	/* umctl2 config */
	struct dram_cfg_param *ddrc_cfg;
	unsigned int ddrc_cfg_num;
	/* ddrphy config */
	struct dram_cfg_param *ddrphy_cfg;
	unsigned int ddrphy_cfg_num;
	/* ddr fsp train info */
	struct dram_fsp_msg *fsp_msg;
	unsigned int fsp_msg_num;
	/* ddr phy trained CSR */
	struct dram_cfg_param *ddrphy_trained_csr;
	unsigned int ddrphy_trained_csr_num;
	/* ddr phy PIE */
	struct dram_cfg_param *ddrphy_pie;
	unsigned int ddrphy_pie_num;

	unsigned int *ddrphy_cali_table;
	unsigned int ddrphy_cali_num;

	/* initialized drate table */
	unsigned int fsp_table[4];
};

static inline void reg32_write(unsigned long addr, u32 val)
{
	writel(val, addr);
}

static inline u32 reg32_read(unsigned long addr)
{
	return readl(addr);
}

static inline void reg32setbit(unsigned long addr, u32 bit)
{
	setbits_le32(addr, (1 << bit));
}

void ddr_init(struct dram_timing_info *timing_info);

#endif /* __ASM_ARCH_HB_DDR_H__ */

