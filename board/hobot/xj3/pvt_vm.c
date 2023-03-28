/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#include <common.h>
#include <sata.h>
#include <ahci.h>
#include <scsi.h>
#include <mmc.h>
#include <malloc.h>
#include <asm/io.h>

#include <asm/armv8/mmu.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch-x2/ddr.h>
#include <hb_info.h>
#include <scomp.h>
#include <stdio.h>
#include <hb_spacc.h>

#define VM_CMN_CLK_BASE             0xA1009000
#define VM_CMN_CLK_SYNTH_ADDR       (VM_CMN_CLK_BASE + 0x0400)
#define VM_CMN_SDIF_STATUS_ADDR     (VM_CMN_CLK_BASE + 0x0408)
#define VM_CMN_SDIF_ADDR            (VM_CMN_CLK_BASE + 0x040c)

#define VM_SDIF_RDATA_ADDR          (VM_CMN_CLK_BASE + 0x0630)
#define VM_CH_n_SDIF_DATA_ADDR      (VM_CMN_CLK_BASE + 0x0640)

#define PVT_SDIF_BUSY_BIT           BIT(0)
#define PVT_SDIF_LOCK_BIT           BIT(1)

#define EFUSE_BASE             0xA600800D
#define PVT_VM_EFUSE_OFFSET    0x000D
#define PVT_VM_K3_MASK         0xFFFF0000
#define PVT_VM_N0_MASK         0x00000FFF

#define PVT_VM_NUM             16

static int hobot_vm_init_hw(void)
{
    static int hobot_vm_inited = 0;
    u32 val;
    int cnt = 0;

    if (hobot_vm_inited == 1)
        return 0;


    /* Configure Clock Synthesizers from pvt spec, 240MHz to 4MHz  */
    mmio_write_32(VM_CMN_CLK_SYNTH_ADDR, 0x01011D1D);

    debug("VM_CLK_SYN: %08x\n",
                mmio_read_32(VM_CMN_CLK_SYNTH_ADDR));
    debug("VM_SDIF_STATUS: 0x%08x\n",
                mmio_read_32(VM_CMN_SDIF_STATUS_ADDR));

    /* enable continue mode, 488 samples/s */
    val = mmio_read_32(VM_CMN_SDIF_STATUS_ADDR);
    while (val & (PVT_SDIF_BUSY_BIT | PVT_SDIF_LOCK_BIT)) {
        val = mmio_read_32(VM_CMN_SDIF_STATUS_ADDR);
        if (cnt++ > 100) {
            printf("SDIF status busy or lock, status 0x%08x\n", val);
            break;
        }
        udelay(100);
    }

    /* set temperature sensor mode via SDIF */
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x89001000 | (0x1 << 16));
    udelay(10);

    /* read back the temperature sensor mode register */
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x81000000);
    udelay(10);

    val = mmio_read_32(VM_SDIF_RDATA_ADDR);
    debug("get ip_cfg: VM_SDIF_RDATA_ADDR: 0x%08x\n", val);

    /* set ip_ctrl, 0x88000104 is self-clear run once,
     * 0x88000108 run continuously
     */
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x88000508);

    hobot_vm_inited = 1;

    return 0;
}

static int hobot_vm_read_channel(int channel, u32 *val)
{

	if(val == NULL) {
		printf("%s %d Para error of val\n", __func__, __LINE__);
        return -1;
	}
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x890010000 | (channel << 16));
    udelay(1000);
    if (!mmio_read_32(VM_CMN_SDIF_STATUS_ADDR)) {
        printf("%s %d TIMEOUT\n", __func__, __LINE__);
        return -1;
    }
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x8d000040);
    udelay(1000);
    if (!mmio_read_32(VM_CMN_SDIF_STATUS_ADDR)) {
        printf("%s %d TIMEOUT\n", __func__, __LINE__);
        return -1;
    }
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x8a000000);
    udelay(1000);
    if (!mmio_read_32(VM_CMN_SDIF_STATUS_ADDR)) {
        printf("%s %d TIMEOUT\n", __func__, __LINE__);
        return -1;
    }
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x8c00ffff);
    udelay(1000);
    if (!mmio_read_32(VM_CMN_SDIF_STATUS_ADDR)) {
        printf("%s %d TIMEOUT\n", __func__, __LINE__);
        return -1;
    }
    mmio_write_32(VM_CMN_SDIF_ADDR, 0x88000508);
    udelay(1000);
    if (!mmio_read_32(VM_CMN_SDIF_STATUS_ADDR)) {
        printf("%s %d TIMEOUT\n", __func__, __LINE__);
        return -1;
    }
    *val = mmio_read_32(VM_CH_n_SDIF_DATA_ADDR + channel * 0x4);
    return 0;
}

int hobot_vm_read_raw(int channel, u32 *val) {
    int volt = 0;
    long long k = 0, offset = 0;
    int vm_k3;
    int vm_n0;
	int vm_mode;
	int reg;
    u32 ch_val;
	
	reg = mmio_read_32(EFUSE_BASE + 4 * 0x4);
    vm_k3 = ((reg & PVT_VM_K3_MASK) >> 16);
    vm_n0 = (reg & PVT_VM_N0_MASK);
    if (vm_k3 > 0x4000 || vm_k3 < 0x3000 ||
        vm_n0 > 0xb00 || vm_n0 < 0xa00 ) {
        debug("VM Calibrated data Invalid\n");
        vm_mode = 1;
    }

    if (vm_mode == 0) {
        debug("VM Calibrated data detected\n");
        debug("reg:%08x, k3:%d, n0:%d, mode:%d\n",
                    reg, vm_k3, vm_n0, vm_mode);
    }

    hobot_vm_init_hw();
    udelay(10 * 1000);

    if (hobot_vm_read_channel(channel, &ch_val)) {
        return -1;
    }
    if (channel >= 0 && channel < PVT_VM_NUM) {
        if (vm_mode == 0) {
            k = vm_k3 * 25;
            offset = vm_n0 * k;
            volt = (k * (long long)(ch_val) - offset) >> 12;
        } else {
            /* (0.24 * (6 * sapmle - 16387)) * 1000 / 16384 */
            volt = (90000 * ch_val - 245805000) >> 10;
        }

        *val = volt;

        debug("%s channel = %d, ch_val=%d\n", __func__, channel, ch_val);
        debug("%s channel = %d, reg_val=%d\n", __func__, channel, *val);
        printf("%s channel = %d, reg_volt=%d\n", __func__, channel, volt);
    } else {
        return -1;
    }

    return -1;
}
