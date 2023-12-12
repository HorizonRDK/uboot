// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2017 Theobroma Systems Design und Consulting GmbH
 */

// FOR LT8618SXB U3

#include <common.h>
#include <linux/types.h>
#include <clk.h>
#include <display.h>
#include <dm.h>
#include <dw_hdmi.h>
#include <edid.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <i2c.h>
#include <power/regulator.h>

#include <asm/armv8/mmu.h>
#include <asm/arch-xj3/hb_reg.h>
#include <asm/arch-xj3/hb_pinmux.h>
#include <asm/arch/hb_pmu.h>
#include <asm/arch/hb_sysctrl.h>
#include <asm/arch-x2/ddr.h>
#include <hb_info.h>

#include "lontium_lt8618sxb.h"
#include "hobot_iar.h"

bool Use_DDRCLK = 0; // 1: DDR mode; 0: SDR (normal) mode
int Resolution_Num = 0;
int CLK_bound = 0;
int VIC_Num = 0;
int I2CADR = 0;
int flag_Ver_u3 = 0;
int CLK_Cnt = 0;
int i2c_bus = 0;

//--------------------------------------------------------//
#define _Read_TV_EDID_
#ifdef _Read_TV_EDID_
u8 Sink_EDID[256];
u8 Sink_EDID2[256];
#endif
//--------------------------------------------------------//

#ifdef _External_sync_
u16 hfp, hs_width, hbp, h_act, h_tal, v_act, v_tal, vfp, vs_width, vbp;
bool hs_pol, vs_pol;
#else
u16 h_act, h_tal, v_act;
#endif
#define msleep(a) udelay(a * 1000)
// 如果 BT1120 是单沿采样（SDR）的话，输入的CLK就是pixel CLK；
// 如果是双沿采样（DDR）的话，输入的CLK就是pixel CLK的一半。
struct udevice *i2c_dev;

u16 IIS_N[] = {
	4096,  // 32K
	6272,  // 44.1K
	6144,  // 48K
	12544, // 88.2K
	12288, // 96K
	25088, // 176K
	24576  // 196K
};

u16 Sample_Freq[] = {
	0x30, // 32K
	0x00, // 44.1K
	0x20, // 48K
	0x80, // 88.2K
	0xa0, // 96K
	0xc0, // 176K
	0xe0  //  196K
};

int Format_Timing[][14] = {
	// H_FP / H_sync / H_BP / H_act / H_total / V_FP / V_sync / V_BP /
	// V_act / V_total / Vic / Pic_Ratio / Clk_bound(SDR) / Clk_bound(DDR)
	{16, 62, 60, 720, 858, 9, 6, 30, 480, 525, 2,
	 _4_3_, _Less_than_50M, _Less_than_50M}, // 480P 60Hz 27MHz 4:3
	// { 16,    62,  60,  720,    858,  9, 6, 30, 480,  525,    3,
	// _16_9_, _Less_than_50M,     _Less_than_50M\},    // 480P 60Hz 27MHz 16:9
	{12, 64, 68, 720, 864, 5, 5, 39, 576, 625, 17,
	 _4_3_, _Less_than_50M, _Less_than_50M}, // 576P 50Hz 27MHz 4:3
	// { 12,    64,  68,  720,    864,  5, 5, 39, 576,  625,    18,
	// _16_9_, _Less_than_50M,     _Less_than_50M},    // 576P 50Hz 27MHz 16:9
	{110, 40, 220, 1280, 1650, 5, 5, 20, 720, 750, 4,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 720P 60Hz 74.25MHz
	{440, 40, 220, 1280, 1980, 5, 5, 20, 720, 750, 19,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 720P 50Hz 74.25MHz
	{1760, 40, 220, 1280, 3300, 5, 5, 20, 720, 750, 62,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 720P 30Hz 74.25MHz
	{2420, 40, 220, 1280, 3960, 5, 5, 20, 720, 750, 61,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 720P 25Hz 74.25MHz
	{88, 44, 148, 1920, 2200, 4, 5, 36, 1080, 1125, 16,
	 _16_9_, _Greater_than_100M, _Bound_50_100M}, // 1080P  60Hz 148.5MHz
	{528, 44, 148, 1920, 2640, 4, 5, 36, 1080, 1125, 31,
	 _16_9_, _Greater_than_100M, _Bound_50_100M}, // 1080P  50Hz 148.5MHz
	{88, 44, 148, 1920, 2200, 4, 5, 36, 1080, 1125, 34,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 1080P  30Hz 74.25MHz
	{528, 44, 148, 1920, 2640, 4, 5, 36, 1080, 1125, 33,
	 _16_9_, _Bound_50_100M, _Less_than_50M}, // 1080P  25Hz 74.25MHz
	// { 638,    44,     148, 1920, 2750, 4, 5,     36, 1080, 1125, 33,
	// _16_9_, _Bound_50_100M,     _Less_than_50M},  // 1080P  24Hz 74.25MHz
	{88, 44, 148, 1920, 2200, 2, 5, 15, 540, 562, 5, _16_9_,
	 _Bound_50_100M, _Less_than_50M}, // 1080i  60Hz 74.25MHz
	{528, 44, 148, 1920, 2640, 2, 5, 25, 540, 562, 20, _16_9_,
	 _Bound_50_100M, _Less_than_50M}, // 1080i  50Hz 74.25MHz
	{176, 88, 296, 3840, 4400, 8, 10, 72, 2160, 2250, 95, _16_9_,
	 _Greater_than_100M, _Greater_than_100M}, // 4K 30Hz    297MHz
	{40, 128, 88, 800, 1056, 1, 4, 23, 600, 628, 0, _4_3_,
	 _Less_than_50M, _Less_than_50M}, // VESA 800x600 40MHz
	{24, 136, 160, 1024, 1344, 3, 6, 29, 768, 806, 0, _4_3_,
	 _Bound_50_100M, _Less_than_50M}, // VESA 1024X768 65MHz
	{5, 13, 270, 1024, 1312, 2, 3, 17, 600, 622, 0, _4_3_,
	 _Less_than_50M, _Less_than_50M}, // VESA 1024X600 65MHz
	{44, 88, 124, 800, 1056, 3, 6, 46, 480, 535, 0, _4_3_,
	 _Less_than_50M, _Less_than_50M}, // VESA 800X480 65MHz
	{70, 143, 213, 1366, 1792, 3, 3, 24, 768, 798, 0, _16_9_,
	 _Bound_50_100M, _Bound_50_100M}, // VESA 800X480 65MHz
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0}, // default
};

u8 LT8618SXB_PLL_u3[3][3] = {
	{0x00, 0x9e, 0xaa}, // < 50MHz
	{0x00, 0x9e, 0x99}, // 50 ~ 100M
	{0x00, 0x9e, 0x88}, // > 100M
};

static int hobot_write_lt8618sxb(uint8_t reg_addr, uint8_t reg_val)
{
	return dm_i2c_reg_write(i2c_dev, reg_addr, reg_val);
}

static uint8_t hobot_read_lt8618sxb(uint8_t reg)
{
	return (dm_i2c_reg_read(i2c_dev, reg));
}

void Debug_DispNum(u8 msg)
{
	xj3_display_debug("0x%x\n", msg);
}

static int LT8618SXB_Chip_ID(void)
{

	int ret, count = 0;
	uint8_t id0 = 0, id1 = 0, id2 = 0;
	ret = hobot_write_lt8618sxb(0xFF, 0x80);
	if (ret)
	{
		printf("hobot write lt8618sxb err!\n");
		return ret;
	}
	ret = hobot_write_lt8618sxb(0xee, 0x01);
	if (ret)
	{
		printf("hobot write lt8618sxb err!\n");
		return ret;
	}

	while (1)
	{
		id0 = hobot_read_lt8618sxb(0x00);
		id1 = hobot_read_lt8618sxb(0x01);
		id2 = hobot_read_lt8618sxb(0x02);
		if (id0 == 0x17 && id1 == 0x02 && (id2 & 0xfc) == 0xe0)
		{
			break;
		}
		count++;
		if (count > 5)
		{
			return -1;
		}
		msleep(100);
	}
	if (id2 == 0xe2)
	{
		flag_Ver_u3 = 1; // u3
	}
	else if (id2 == 0xe1)
	{
		flag_Ver_u3 = 0; // u2
	}
	return 0;
}

void LT8618SXB_RST_PD_Init(void)
{
	hobot_write_lt8618sxb(0xff, 0x80);
	hobot_write_lt8618sxb(0x11, 0x00); // reset MIPI Rx logic.
	hobot_write_lt8618sxb(0x13, 0xf1);
	hobot_write_lt8618sxb(0x13, 0xf9); // Reset TTL video process
}

void LT8618SXB_TTL_Input_Digtal(void)
{
	hobot_write_lt8618sxb(0xff, 0x80);
#ifdef _Embedded_sync_
	// Internal generate sync/de control logic clock enable
	hobot_write_lt8618sxb(0x0A, 0xF0);
#else // _External_sync_
	hobot_write_lt8618sxb(0x0A, 0xC0);
#endif

	// TTL_Input_Digtal
	hobot_write_lt8618sxb(0xff, 0x82);		   // register bank
	hobot_write_lt8618sxb(0x45, _YC_Channel_); // YC channel swap
	hobot_write_lt8618sxb(0x46, _Reg0x8246_);
	hobot_write_lt8618sxb(0x50, 0x00);
	//*
	if (Use_DDRCLK)
	{
		hobot_write_lt8618sxb(0x4f, 0x80); // 0x80: dclk
	}
	else
	{
		hobot_write_lt8618sxb(0x4f, 0x40); // 0x40: txpll_clk
	}

	// hobot_write_lt8618sxb(0x4f, 0x40);

#ifdef _Embedded_sync_
	hobot_write_lt8618sxb(0x51, 0x42); // Select BT rx decode det_vs/hs/de
	// Embedded sync mode input enable.
	hobot_write_lt8618sxb(0x48, 0x08 + _Reg0x8248_D1_D0_);
#else // _External_sync_
	// Select TTL process module input video data
	hobot_write_lt8618sxb(0x51, 0x00);
	hobot_write_lt8618sxb(0x48, 0x00 + _Reg0x8248_D1_D0_);
#endif
}

void LT8618SXB_Video_check(void)
{
	hobot_write_lt8618sxb(0xff, 0x82); // video
#ifdef _Embedded_sync_
	h_tal = hobot_read_lt8618sxb(0x8f);
	h_tal = (h_tal << 8) + hobot_read_lt8618sxb(0x90);

	v_act = hobot_read_lt8618sxb(0x8b);
	v_act = (v_act << 8) + hobot_read_lt8618sxb(0x8c);

	h_act = hobot_read_lt8618sxb(0x8d);
	h_act = (h_act << 8) + hobot_read_lt8618sxb(0x8e);
	CLK_Cnt = (hobot_read_lt8618sxb(0x1d) & 0x0f) *
				  0x10000 +
			  hobot_read_lt8618sxb(0x1e) *
				  0x100 +
			  hobot_read_lt8618sxb(0x1f);
	xj3_display_debug("\r\nRead LT8618SXB Video Check:");

	xj3_display_debug("\r\n ");
	xj3_display_debug("\r\n h_act, h_tol = ");
	Debug_DispNum(h_act);
	xj3_display_debug(", ");
	Debug_DispNum(h_tal);

	xj3_display_debug("\r\n ");
	xj3_display_debug("\r\nv_act = ");
	Debug_DispNum(v_act);
	xj3_display_debug("\r\n ");
	xj3_display_debug("\r\nclk = %d\n", CLK_Cnt);
#else
	u8 temp;

	vs_pol = 0;
	hs_pol = 0;

	// _External_sync_
	temp = hobot_read_lt8618sxb(0x70); // hs vs polarity
	if (temp & 0x02)
	{
		vs_pol = 1;
	}
	if (temp & 0x01)
	{
		hs_pol = 1;
	}

	vs_width = hobot_read_lt8618sxb(0x71);

	hs_width = hobot_read_lt8618sxb(0x72);
	hs_width = (hs_width << 8) + hobot_read_lt8618sxb(0x73);

	vbp = hobot_read_lt8618sxb(0x74);
	vfp = hobot_read_lt8618sxb(0x75);

	hbp = hobot_read_lt8618sxb(0x76);
	hbp = (hbp << 8) + hobot_read_lt8618sxb(0x77);

	hfp = hobot_read_lt8618sxb(0x78);
	hfp = (hfp << 8) + hobot_read_lt8618sxb(0x79);

	v_tal = hobot_read_lt8618sxb(0x7a);
	v_tal = (v_tal << 8) + hobot_read_lt8618sxb(0x7b);

	h_tal = hobot_read_lt8618sxb(0x7c);
	h_tal = (h_tal << 8) + hobot_read_lt8618sxb(0x7d);

	v_act = hobot_read_lt8618sxb(0x7e);
	v_act = (v_act << 8) + hobot_read_lt8618sxb(0x7f);

	h_act = hobot_read_lt8618sxb(0x80);
	h_act = (h_act << 8) + hobot_read_lt8618sxb(0x81);

	CLK_Cnt =
		(hobot_read_lt8618sxb(0x1d) & 0x0f) * \\0x10000 +
		hobot_read_lt8618sxb(0x1e) * \\0x100 + hobot_read_lt8618sxb(0x1f);

	xj3_display_debug("\r\nRead LT8618SXB Video Check:");

	xj3_display_debug("\r\n ");
	xj3_display_debug("\r\nh_fp, hs_wid, h_bp, h_act, h_tol = ");
	Debug_DispNum(hfp);
	xj3_display_debug(", ");
	Debug_DispNum(hs_width);
	xj3_display_debug(", ");
	Debug_DispNum(hbp);
	xj3_display_debug(", ");
	Debug_DispNum(h_act);
	xj3_display_debug(", ");
	Debug_DispNum(h_tal);

	xj3_display_debug("\r\n ");
	xj3_display_debug("\r\nv_fp, vs_wid, v_bp, v_act, v_tol = ");
	Debug_DispNum(vfp);
	xj3_display_debug(", ");
	Debug_DispNum(vs_width);
	xj3_display_debug(", ");
	Debug_DispNum(vbp);
	xj3_display_debug(", ");
	Debug_DispNum(v_act);
	xj3_display_debug(", ");
	Debug_DispNum(v_tal);
#endif
}

void LT8618SXB_PLL_setting(void)
{
	// xj3_display_debug("*****LT8618SXB_PLL_setting*****\n");
	u8 read_val;
	u8 j;
	u8 cali_done;
	u8 cali_val;
	u8 lock;

	// if (g_x2_lt8618sxb == NULL)
	// 	return -1;

	CLK_bound =
		(u8)Format_Timing[Resolution_Num][Clk_bound_SDR +
										  (u8)(Use_DDRCLK)];

	hobot_write_lt8618sxb(0xff, 0x81);
	hobot_write_lt8618sxb(0x23, 0x40);
	if (flag_Ver_u3)
	{
		hobot_write_lt8618sxb(0x24, 0x62); // icp set
	}
	else
	{
		hobot_write_lt8618sxb(0x24, 0x64); // icp set
	}
	hobot_write_lt8618sxb(0x26, 0x55);

	hobot_write_lt8618sxb(0x29, 0x04); // for U3 for U3 SDR/DDR fixed phase

	if (flag_Ver_u3)
	{
		hobot_write_lt8618sxb(0x25, LT8618SXB_PLL_u3[CLK_bound][0]);
		hobot_write_lt8618sxb(0x2c, LT8618SXB_PLL_u3[CLK_bound][1]);
		hobot_write_lt8618sxb(0x2d, LT8618SXB_PLL_u3[CLK_bound][2]);
	}
	// else
	// {
	// 	hobot_write_lt8618sxb(0x25, LT8618SXB_PLL_u2[CLK_bound][0]);
	// 	hobot_write_lt8618sxb(0x2c, LT8618SXB_PLL_u2[CLK_bound][1]);
	// 	hobot_write_lt8618sxb(0x2d, LT8618SXB_PLL_u2[CLK_bound][2]);
	// }

	if (Use_DDRCLK)
	{
		if (flag_Ver_u3)
		{
			hobot_write_lt8618sxb(0x4d, 0x05);
			hobot_write_lt8618sxb(0x27, 0x60); // 0x60 //ddr 0x66
			hobot_write_lt8618sxb(0x28, 0x88);
		}
		else
		{
			read_val = hobot_read_lt8618sxb(0x2c) & 0x7f;
			read_val = read_val * 2 | 0x80;
			hobot_write_lt8618sxb(0x2c, read_val);

			hobot_write_lt8618sxb(0x4d, 0x04);
			hobot_write_lt8618sxb(0x27, 0x60); // 0x60
			hobot_write_lt8618sxb(0x28, 0x88);
		}

#ifdef _LOG_
		xj3_display_debug("\r\n PLL DDR");
#endif
	}
	else
	{
		if (flag_Ver_u3)
		{
			hobot_write_lt8618sxb(0x4d, 0x0d);
			hobot_write_lt8618sxb(0x27, 0x60); // 0x06
			hobot_write_lt8618sxb(0x28, 0x88); // 0x88
		}
		else
		{
			hobot_write_lt8618sxb(0x4d, 0x00);
			hobot_write_lt8618sxb(0x27, 0x60); // 0x06
			hobot_write_lt8618sxb(0x28, 0x00); // 0x88
		}

#ifdef _LOG_
		xj3_display_debug("\r\n PLL SDR");
#endif
	}

	hobot_write_lt8618sxb(0xff, 0x81);

	read_val = hobot_read_lt8618sxb(0x2b);
	hobot_write_lt8618sxb(0x2b, read_val & 0xfd); // sw_en_txpll_cal_en

	read_val = hobot_read_lt8618sxb(0x2e);
	hobot_write_lt8618sxb(0x2e, read_val & 0xfe); // sw_en_txpll_iband_set

	hobot_write_lt8618sxb(0xff, 0x82);
	hobot_write_lt8618sxb(0xde, 0x00);
	hobot_write_lt8618sxb(0xde, 0xc0);

	hobot_write_lt8618sxb(0xff, 0x80);
	hobot_write_lt8618sxb(0x16, 0xf1);
	hobot_write_lt8618sxb(0x18, 0xdc); // txpll _sw_rst_n
	hobot_write_lt8618sxb(0x18, 0xfc);
	hobot_write_lt8618sxb(0x16, 0xf3);

	hobot_write_lt8618sxb(0xff, 0x81);

	// #ifdef _Ver_U3_
	if (flag_Ver_u3)
	{
		if (Use_DDRCLK)
		{
			hobot_write_lt8618sxb(0x2a, 0x10);
			hobot_write_lt8618sxb(0x2a, 0x30);
		}
		else
		{
			hobot_write_lt8618sxb(0x2a, 0x00);
			hobot_write_lt8618sxb(0x2a, 0x20);
		}
	}
	// #endif

	for (j = 0; j < 0x05; j++)
	{
		msleep(10);
		hobot_write_lt8618sxb(0xff, 0x80);
		hobot_write_lt8618sxb(0x16, 0xe3); /* pll lock logic reset */
		hobot_write_lt8618sxb(0x16, 0xf3);

		hobot_write_lt8618sxb(0xff, 0x82);
		lock = 0x80 & hobot_read_lt8618sxb(0x15);
		cali_val = hobot_read_lt8618sxb(0xea);
		cali_done = 0x80 & hobot_read_lt8618sxb(0xeb);

		if (lock && cali_done && (cali_val != 0xff) && (cali_val >= 0x20))
		{
#ifdef _LOG_
			xj3_display_debug("TXPLL Lock");
			hobot_write_lt8618sxb(0xff, 0x82);
			xj3_display_debug("0x82ea=%x\n", hobot_read_lt8618sxb(0xea));
			xj3_display_debug("0x82eb=%x\n", hobot_read_lt8618sxb(0xeb));
			xj3_display_debug("0x82ec=%x\n", hobot_read_lt8618sxb(0xec));
			xj3_display_debug("0x82ed=%x\n", hobot_read_lt8618sxb(0xed));
			xj3_display_debug("0x82ee=%x\n", hobot_read_lt8618sxb(0xee));
			xj3_display_debug("0x82ef=%x\n", hobot_read_lt8618sxb(0xef));

#endif
		}
		else
		{
			hobot_write_lt8618sxb(0xff, 0x80);
			hobot_write_lt8618sxb(0x16, 0xf1);
			hobot_write_lt8618sxb(0x18, 0xdc); // txpll _sw_rst_n
			hobot_write_lt8618sxb(0x18, 0xfc);
			hobot_write_lt8618sxb(0x16, 0xf3);
#ifdef _LOG_
			xj3_display_debug("TXPLL Reset\n");
#endif
		}
	}
}
void LT8618SXB_Audio_setting(void)
{
	// xj3_display_debug("*****LT8618SXB_Audio_setting*****\n");
	//----------------IIS-----------------------
	//  IIS Input
	hobot_write_lt8618sxb(0xff, 0x82); // register bank
	// bit7 = 0 : DVI output; bit7 = 1: HDMI output
	hobot_write_lt8618sxb(0xd6, 0x0e);
	hobot_write_lt8618sxb(0xd7, 0x04); // sync polarity

	hobot_write_lt8618sxb(0xff, 0x84); // register bank
	hobot_write_lt8618sxb(0x06, 0x08);
	hobot_write_lt8618sxb(0x07, 0x10); // SD0 channel selected

	hobot_write_lt8618sxb(0x09, 0x00); // 0x00 :Left justified; default
	// 0x02 :Right justified;

	//-----------------SPDIF---------------------

	/*    SPDIF Input
		  hobot_write_lt8618sxb(0xff, 0x82);// register bank
		  hobot_write_lt8618sxb(0xd6, 0x8e);
		  hobot_write_lt8618sxb(0xd7, 0x00);    //sync polarity

		  hobot_write_lt8618sxb(0xff, 0x84);// register bank
		  hobot_write_lt8618sxb(0x06, 0x0c);
		  hobot_write_lt8618sxb(0x07, 0x10);
	// */

	//-------------------------------------------

	hobot_write_lt8618sxb(0x0f, 0x0b + Sample_Freq[_48KHz]);

	// 根据音频数据长度的不同，设置不同的寄存器值。
	hobot_write_lt8618sxb(0x34, 0xd4); // CTS_N / 2; 32bit
	//    hobot_write_lt8618sxb(0x34, 0xd5);    //CTS_N / 4; 16bit

	hobot_write_lt8618sxb(0x35, (u8)(IIS_N[_48KHz] / 0x10000));
	hobot_write_lt8618sxb(0x36, (u8)((IIS_N[_48KHz] & 0x00FFFF) / 0x100));
	hobot_write_lt8618sxb(0x37, (u8)(IIS_N[_48KHz] & 0x0000FF));

	hobot_write_lt8618sxb(0x3c, 0x21); // Null packet enable
}

/***********************************************************

 ***********************************************************/

// LT8618SXB only supports three color space convert: YUV422, yuv444 and rgb888.
// Color space convert of YUV420 is not supported.
void LT8618SXB_CSC_setting(void)
{
	// xj3_display_debug("*****LT8618SXB_CSC_setting*****\n");
	//  color space config
	hobot_write_lt8618sxb(0xff, 0x82); // register bank
	//    hobot_write_lt8618sxb(0xb9, 0x08);// YCbCr444 to RGB
	hobot_write_lt8618sxb(0xb9, 0x18); // YCbCr422 to RGB

	//    hobot_write_lt8618sxb(0xb9, 0x80);// RGB to YCbCr444
	//    hobot_write_lt8618sxb(0xb9, 0xa0);// RGB to YCbCr422

	//    hobot_write_lt8618sxb(0xb9, 0x10);// YCbCr422 to YCbCr444
	//    hobot_write_lt8618sxb(0xb9, 0x20);// YCbCr444 to YCbCr422

	//    hobot_write_lt8618sxb(0xb9, 0x00); // No csc
}

/***********************************************************

 ***********************************************************/
void LT8618SXB_AVI_setting(void)
{
	//    xj3_display_debug("*****LT8618SXB_AVI_setting*****\n");
	// AVI
	u8 AVI_PB0 = 0x00;
	u8 AVI_PB1 = 0x00;
	u8 AVI_PB2 = 0x00;

	/*************************************************************************
	  The 0x43 register is checksums,
	  changing the value of the 0x45 or 0x47 register,
	  and the value of the 0x43 register is also changed.
	  0x43, 0x44, 0x45, and 0x47 are the sum of the four register values is 0x6F.
	 *************************************************************************/

	//    VIC_Num = 0x04; // 720P 60; Corresponding to the resolution to be output
	//    VIC_Num = 0x10;    // 1080P 60
	//    VIC_Num = 0x1F;    // 1080P 50
	//    VIC_Num = 0x5F;    // 4K30;

	//================================================================//

	// Please refer to function: void LT8618SXB_CSC_setting(void)

	/****************************************************
	  Because the color space of BT1120 is YUV422,
	  if lt8618sxb does not do color space convert (no CSC),
	  the color space of output HDMI is YUV422.
	 *****************************************************/

	//	VIC_Num = 0x10;

	// if No csc
	// AVI_PB1 = 0x30;  // PB1,color space: YUV444 0x50;YUV422 0x30; RGB 0x10

	// if YCbCr422 to RGB CSC
	AVI_PB1 = 0x10; // PB1,color space: YUV444 0x50;YUV422 0x30; RGB 0x10

	// PB2; picture aspect rate: 0x19:4:3 ;     0x2A : 16:9
	AVI_PB2 = Format_Timing[Resolution_Num][Pic_Ratio];

	AVI_PB0 = ((AVI_PB1 + AVI_PB2 + VIC_Num) <= 0x6f) ? (0x6f - AVI_PB1 - AVI_PB2 - VIC_Num) : (0x16f - AVI_PB1 - AVI_PB2 - VIC_Num);
	// register bank
	hobot_write_lt8618sxb(0xff, 0x84);
	// PB0,avi packet checksum
	hobot_write_lt8618sxb(0x43, AVI_PB0);
	// PB1,color space: YUV444 0x50;YUV422 0x30; RGB 0x10
	hobot_write_lt8618sxb(0x44, AVI_PB1);
	// PB2;picture aspect rate: 0x19:4:3 ; 0x2A : 16:9
	hobot_write_lt8618sxb(0x45, AVI_PB2);
	// PB4;vic ,0x10: 1080P ;  0x04 : 720P
	hobot_write_lt8618sxb(0x47, VIC_Num);

	//    hobot_write_lt8618sxb(0xff,0x84); 
	//    8618SXB hdcp1.4 加密的话，
	//    要保证hfp + 8410[5:0](rg_island_tr_res) 的个数 
	//   （video de的下降沿到 最近的一个aude 的间隔），
	//    大于58；hdcp1.4 spec中有要求
	//    hobot_write_lt8618sxb(0x10, 0x30);             //data iland
	//    hobot_write_lt8618sxb(0x12, 0x64);             //act_h_blank

	// VS_IF, 4k 30hz need send VS_IF packet. Please refer to hdmi1.4 spec 8.2.3
	if (VIC_Num == 95)
	{
		//       hobot_write_lt8618sxb(0xff,0x84);
		hobot_write_lt8618sxb(0x3d, 0x2a); // UD1 infoframe enable

		hobot_write_lt8618sxb(0x74, 0x81);
		hobot_write_lt8618sxb(0x75, 0x01);
		hobot_write_lt8618sxb(0x76, 0x05);
		hobot_write_lt8618sxb(0x77, 0x49);
		hobot_write_lt8618sxb(0x78, 0x03);
		hobot_write_lt8618sxb(0x79, 0x0c);
		hobot_write_lt8618sxb(0x7a, 0x00);
		hobot_write_lt8618sxb(0x7b, 0x20);
		hobot_write_lt8618sxb(0x7c, 0x01);
	}
	else
	{
		//       hobot_write_lt8618sxb(0xff,0x84);
		hobot_write_lt8618sxb(0x3d, 0x0a); // UD1 infoframe disable
	}
}

/***********************************************************

 ***********************************************************/
void LT8618SXB_TX_Phy(void)
{
	//    xj3_display_debug("*****LT8618SXB_TX_Phy*****\n");

	// HDMI_TX_Phy
	hobot_write_lt8618sxb(0xff, 0x81); // register bank
	hobot_write_lt8618sxb(0x30, 0xea);
#if 1								   // DC mode
	hobot_write_lt8618sxb(0x31, 0x44); // HDMDITX_dc_det_en 0x44
	hobot_write_lt8618sxb(0x32, 0x4a);
	hobot_write_lt8618sxb(0x33, 0x0b); // 0x0b
#else								   // AC mode
	hobot_write_lt8618sxb(0x31, 0x73); // HDMDITX_dc_det_en 0x44
	hobot_write_lt8618sxb(0x32, 0xea);
	hobot_write_lt8618sxb(0x33, 0x4a); // 0x0b
#endif

	hobot_write_lt8618sxb(0x34, 0x00);
	hobot_write_lt8618sxb(0x35, 0x00);
	hobot_write_lt8618sxb(0x36, 0x00);
	hobot_write_lt8618sxb(0x37, 0x44);
	hobot_write_lt8618sxb(0x3f, 0x0f);

	hobot_write_lt8618sxb(0x40, 0xb0); // 0xa0 -- CLK tap0 swing
	hobot_write_lt8618sxb(0x41, 0x68); // 0xa0 -- D0 tap0 swing
	hobot_write_lt8618sxb(0x42, 0x68); // 0xa0 -- D1 tap0 swing
	hobot_write_lt8618sxb(0x43, 0x68); // 0xa0 -- D2 tap0 swing

	hobot_write_lt8618sxb(0x44, 0x0a);
}

/***********************************************************

 ***********************************************************/

u8 LT8618SX_Phase_1(void)
{
	u8 temp = 0;
	u8 read_val = 0;

	u8 OK_CNT = 0x00;
	u8 OK_CNT_1 = 0x00;
	u8 OK_CNT_2 = 0x00;
	u8 OK_CNT_3 = 0x00;
	u8 Jump_CNT = 0x00;
	u8 Jump_Num = 0x00;
	u8 Jump_Num_1 = 0x00;
	u8 Jump_Num_2 = 0x00;
	u8 Jump_Num_3 = 0x00;
	bool temp0_ok = 0;
	bool temp9_ok = 0;
	bool b_OK = 0;

	hobot_write_lt8618sxb(0xff, 0x80); // register bank
	hobot_write_lt8618sxb(0x13, 0xf1);
	msleep(5);
	hobot_write_lt8618sxb(0x13, 0xf9); // Reset TTL video process
	msleep(10);
	hobot_write_lt8618sxb(0xff, 0x81);

	for (temp = 0; temp < 0x0a; temp++)
	{
		hobot_write_lt8618sxb(0x27, (0x60 + temp));
		// #ifdef _DDR_
		if (Use_DDRCLK)
		{
			hobot_write_lt8618sxb(0x4d, 0x05);
			msleep(5);
			hobot_write_lt8618sxb(0x4d, 0x0d);
			//    msleep(50);
		}
		else
		{
			hobot_write_lt8618sxb(0x4d, 0x01);
			msleep(5);
			hobot_write_lt8618sxb(0x4d, 0x09);
		}
		// #endif
		msleep(10);
		read_val = hobot_read_lt8618sxb(0x50) & 0x01;

#ifdef _Phase_Debug_
		xj3_display_debug("\r\ntemp=");
		Debug_DispNum(temp);
		xj3_display_debug("\r\nread_val=");
		Debug_DispNum(read_val);
#endif

		if (read_val == 0)
		{
			OK_CNT++;

			if (b_OK == 0)
			{
				b_OK = 1;
				Jump_CNT++;

				if (Jump_CNT == 1)
				{
					Jump_Num_1 = temp;
				}
				else if (Jump_CNT == 3)
				{
					Jump_Num_2 = temp;
				}
				else if (Jump_CNT == 5)
				{
					Jump_Num_3 = temp;
				}
			}

			if (Jump_CNT == 1)
			{
				OK_CNT_1++;
			}
			else if (Jump_CNT == 3)
			{
				OK_CNT_2++;
			}
			else if (Jump_CNT == 5)
			{
				OK_CNT_3++;
			}

			if (temp == 0)
			{
				temp0_ok = 1;
			}
			if (temp == 9)
			{
				Jump_CNT++;
				temp9_ok = 1;
			}
		}
		else
		{
			if (b_OK)
			{
				b_OK = 0;
				Jump_CNT++;
			}
		}
	}

	if ((Jump_CNT == 0) || (Jump_CNT > 6))
	{
#ifdef _Phase_Debug_
		xj3_display_debug("\r\ncali phase fail");
#endif
		return 0;
	}

	if ((temp9_ok == 1) && (temp0_ok == 1))
	{
		if (Jump_CNT == 6)
		{
			OK_CNT_3 = OK_CNT_3 + OK_CNT_1;
			OK_CNT_1 = 0;
		}
		else if (Jump_CNT == 4)
		{
			OK_CNT_2 = OK_CNT_2 + OK_CNT_1;
			OK_CNT_1 = 0;
		}
	}
	if (Jump_CNT >= 2)
	{
		if (OK_CNT_1 >= OK_CNT_2)
		{
			if (OK_CNT_1 >= OK_CNT_3)
			{
				OK_CNT = OK_CNT_1;
				Jump_Num = Jump_Num_1;
			}
			else
			{
				OK_CNT = OK_CNT_3;
				Jump_Num = Jump_Num_3;
			}
		}
		else
		{
			if (OK_CNT_2 >= OK_CNT_3)
			{
				OK_CNT = OK_CNT_2;
				Jump_Num = Jump_Num_2;
			}
			else
			{
				OK_CNT = OK_CNT_3;
				Jump_Num = Jump_Num_3;
			}
		}
	}

	hobot_write_lt8618sxb(0xff, 0x81);

	if ((Jump_CNT == 2) || (Jump_CNT == 4) || (Jump_CNT == 6))
	{
		hobot_write_lt8618sxb(0x27,
							  (0x60 +
							   (Jump_Num + (OK_CNT / 2)) % 0x0a));
	}
	else if (OK_CNT >= 0x09)
	{
		hobot_write_lt8618sxb(0x27, 0x65);
	}
#ifdef _Phase_Debug_
	//    xj3_display_debug("\r\nRead LT8618SXB ID ");
	//    xj3_display_debug("\r\n ");
	xj3_display_debug("\r\nReg0x8127 = : 0x%x", hobot_read_lt8618sxb(0x27));
	//    xj3_display_debug(" ", HDMI_ReadI2C_Byte(0x01));
	//    xj3_display_debug(" ", HDMI_ReadI2C_Byte(0x02));
	//    xj3_display_debug("\r\n ");
#endif

	return 1;
}

/***********************************************************

 ***********************************************************/
#ifdef _Embedded_sync_
void LT8618SXB_BT_Timing_setting(void)
{
	// xj3_display_debug("*****LT8618SXB_BT_Timing_setting*****\n");

	// BT1120 内同步信号只有DE, 如果没有外部sync 和DE, 
	// Video check 只能检测到H total 和 H/V active.
	// BT1120 的Timing 值设置就不能通过Video check读取timing状态 
	// 寄存器的值来设置，需要根据前端BT1120分辨率来设置 
	// The synchronization signal in BT1120 is only DE.
	// Without external sync and DE, Video check can only 
	// detect H total and H/V active.
	// BT1120 Timing value settings can not be set by reading 
	// the value of the timing status register 
	// through Video check, which needs to be set according to 
	// the front BT1120 resolution.

	// LT8618SX_BT1120_Set
	// Pls refer to array : Format_Timing[][14]
	hobot_write_lt8618sxb(0xff, 0x82);
	hobot_write_lt8618sxb(0x20,
						  (u8)(Format_Timing[Resolution_Num][H_act] /
							   256));
	hobot_write_lt8618sxb(0x21,
						  (u8)(Format_Timing[Resolution_Num][H_act] %
							   256));
	hobot_write_lt8618sxb(0x22,
						  (u8)(Format_Timing[Resolution_Num][H_fp] / 256));
	hobot_write_lt8618sxb(0x23,
						  (u8)(Format_Timing[Resolution_Num][H_fp] % 256));
	hobot_write_lt8618sxb(0x24,
						  (u8)(Format_Timing[Resolution_Num][H_sync] /
							   256));
	hobot_write_lt8618sxb(0x25,
						  (u8)(Format_Timing[Resolution_Num][H_sync] %
							   256));
	hobot_write_lt8618sxb(0x26, 0x00);
	hobot_write_lt8618sxb(0x27, 0x00);
	hobot_write_lt8618sxb(0x36,
						  (u8)(Format_Timing[Resolution_Num][V_act] /
							   256));
	hobot_write_lt8618sxb(0x37,
						  (u8)(Format_Timing[Resolution_Num][V_act] %
							   256));
	hobot_write_lt8618sxb(0x38,
						  (u8)(Format_Timing[Resolution_Num][V_fp] / 256));
	hobot_write_lt8618sxb(0x39,
						  (u8)(Format_Timing[Resolution_Num][V_fp] % 256));
	hobot_write_lt8618sxb(0x3a,
						  (u8)(Format_Timing[Resolution_Num][V_bp] / 256));
	hobot_write_lt8618sxb(0x3b,
						  (u8)(Format_Timing[Resolution_Num][V_bp] % 256));
	hobot_write_lt8618sxb(0x3c,
						  (u8)(Format_Timing[Resolution_Num][V_sync] /
							   256));
	hobot_write_lt8618sxb(0x3d,
						  (u8)(Format_Timing[Resolution_Num][V_sync] %
							   256));
}

#if 1
u8 LT8618SX_Phase(void)
{
	// xj3_display_debug("*****LT8618SX_Phase*****\n");
	u8 temp = 0;
	u8 read_value = 0;
	u8 b_ok = 0;
	u8 Temp_f = 0;

	for (temp = 0; temp < 0x0a; temp++)
	{
		hobot_write_lt8618sxb(0xff, 0x81);
		hobot_write_lt8618sxb(0x27, (0x60 + temp));

		if (Use_DDRCLK)
		{
			hobot_write_lt8618sxb(0x4d, 0x05);
			msleep(5);
			hobot_write_lt8618sxb(0x4d, 0x0d);
		}
		else
		{
			hobot_write_lt8618sxb(0x4d, 0x01);
			msleep(5);
			hobot_write_lt8618sxb(0x4d, 0x09);
		}

		msleep(50);

		read_value = hobot_read_lt8618sxb(0x50);

#ifdef _Phase_Debug_
		xj3_display_debug("\r\ntemp=");
		Debug_DispNum(temp);
		xj3_display_debug("\r\nread_value=");
		Debug_DispNum(read_value);
#endif
		if (read_value == 0x00)
		{
			if (b_ok == 0)
			{
				Temp_f = temp;
			}
			b_ok = 1;
		}
		else
		{
			b_ok = 0;
		}
	}
#ifdef _Phase_Debug_
	xj3_display_debug("\r\nTemp_f=");
	Debug_DispNum(Temp_f);
#endif
	hobot_write_lt8618sxb(0xff, 0x81);
	hobot_write_lt8618sxb(0x27, (0x60 + Temp_f));

	return Temp_f;
}

/***********************************************************

 ***********************************************************/
bool LT8618SXB_Phase_config(void)
{
	// xj3_display_debug("*****LT8618SXB_Phase_config*****\n");
	u8 Temp = 0x00;
	u8 Temp_f = 0x00;
	u8 OK_CNT = 0x00;
	u8 OK_CNT_1 = 0x00;
	u8 OK_CNT_2 = 0x00;
	u8 OK_CNT_3 = 0x00;
	u8 Jump_CNT = 0x00;
	u8 Jump_Num = 0x00;
	u8 Jump_Num_1 = 0x00;
	u8 Jump_Num_2 = 0x00;
	u8 Jump_Num_3 = 0x00;
	bool temp0_ok = 0;
	bool temp9_ok = 0;
	bool b_OK = 0;
	u16 V_ACT = 0x0000;
	u16 H_ACT = 0x0000;
	u16 H_TOTAL = 0x0000;
	//    u16        V_TOTAL       = 0x0000;
	//    u8        H_double   = 1;

	Temp_f = LT8618SX_Phase(); // it's setted before video check

	while (Temp <= 0x09)
	{
		hobot_write_lt8618sxb(0xff, 0x81);
		hobot_write_lt8618sxb(0x27, (0x60 + Temp));
		hobot_write_lt8618sxb(0xff, 0x80);
		hobot_write_lt8618sxb(0x13, 0xf1); // ttl video process reset///20191121
		hobot_write_lt8618sxb(0x12, 0xfb); // video check reset//20191121
		msleep(5);						   // add 20191121
		hobot_write_lt8618sxb(0x12, 0xff); // 20191121
		hobot_write_lt8618sxb(0x13, 0xf9); // 20191121

		msleep(80); //

		hobot_write_lt8618sxb(0xff, 0x81);
		hobot_write_lt8618sxb(0x51, 0x42);

		hobot_write_lt8618sxb(0xff, 0x82);
		H_TOTAL = hobot_read_lt8618sxb(0x8f);
		H_TOTAL = (H_TOTAL << 8) + hobot_read_lt8618sxb(0x90);
		V_ACT = hobot_read_lt8618sxb(0x8b);
		V_ACT = (V_ACT << 8) + hobot_read_lt8618sxb(0x8c);
		H_ACT = hobot_read_lt8618sxb(0x8d);
		H_ACT = (H_ACT << 8) + hobot_read_lt8618sxb(0x8e) - 0x04; // note

		//                    hobot_write_lt8618sxb(0xff, 0x80);
		//                    hobot_write_lt8618sxb(0x09, 0xfe);

#ifdef _Phase_Debug_
		xj3_display_debug("\r\n h_total=");
		Debug_DispNum(H_TOTAL);
		xj3_display_debug("\r\n v_act=");
		Debug_DispNum(V_ACT);
		xj3_display_debug("\r\n h_act=");
		Debug_DispNum(H_ACT);
#endif
		if ((V_ACT > (Format_Timing[Resolution_Num][V_act] - 5)) && (V_ACT < (Format_Timing[Resolution_Num][V_act] + 5)) && (H_ACT > (Format_Timing[Resolution_Num][H_act] - 5)) && (H_ACT < (Format_Timing[Resolution_Num][H_act] + 5)) && (H_TOTAL > (Format_Timing[Resolution_Num][H_tol] - 5)) && (H_TOTAL < (Format_Timing[Resolution_Num][H_tol] + 5)))
		{
			OK_CNT++;

			if (b_OK == 0)
			{
				b_OK = 1;
				Jump_CNT++;

				if (Jump_CNT == 1)
				{
					Jump_Num_1 = Temp;
				}
				else if (Jump_CNT == 3)
				{
					Jump_Num_2 = Temp;
				}
				else if (Jump_CNT == 5)
				{
					Jump_Num_3 = Temp;
				}
			}

			if (Jump_CNT == 1)
			{
				OK_CNT_1++;
			}
			else if (Jump_CNT == 3)
			{
				OK_CNT_2++;
			}
			else if (Jump_CNT == 5)
			{
				OK_CNT_3++;
			}

			if (Temp == 0)
			{
				temp0_ok = 1;
			}
			if (Temp == 9)
			{
				Jump_CNT++;
				temp9_ok = 1;
			}
#ifdef _Phase_Debug_
			xj3_display_debug("\r\n this phase is ok,temp=");
			Debug_DispNum(Temp);
			xj3_display_debug("\r\n Jump_CNT=");
			Debug_DispNum(Jump_CNT);
#endif
		}
		else
		{
			if (b_OK)
			{
				b_OK = 0;
				Jump_CNT++;
			}
#ifdef _Phase_Debug_
			xj3_display_debug("\r\n this phase is fail,temp=");
			Debug_DispNum(Temp);
			xj3_display_debug("\r\n Jump_CNT=");
			Debug_DispNum(Jump_CNT);
#endif
		}

		Temp++;
	}

#ifdef _Phase_Debug_
	xj3_display_debug("\r\n OK_CNT_1=");
	Debug_DispNum(OK_CNT_1);
	xj3_display_debug("\r\n OK_CNT_2=");
	Debug_DispNum(OK_CNT_2);
	xj3_display_debug("\r\n OK_CNT_3=");
	Debug_DispNum(OK_CNT_3);
#endif

	if ((Jump_CNT == 0) || (Jump_CNT > 6))
	{
#ifdef _Phase_Debug_
		xj3_display_debug("\r\ncali phase fail");
#endif
		return 0;
	}

	if ((temp9_ok == 1) && (temp0_ok == 1))
	{
		if (Jump_CNT == 6)
		{
			OK_CNT_3 = OK_CNT_3 + OK_CNT_1;
			OK_CNT_1 = 0;
		}
		else if (Jump_CNT == 4)
		{
			OK_CNT_2 = OK_CNT_2 + OK_CNT_1;
			OK_CNT_1 = 0;
		}
	}

	if (Jump_CNT >= 2)
	{
		if (OK_CNT_1 >= OK_CNT_2)
		{
			if (OK_CNT_1 >= OK_CNT_3)
			{
				OK_CNT = OK_CNT_1;
				Jump_Num = Jump_Num_1;
			}
			else
			{
				OK_CNT = OK_CNT_3;
				Jump_Num = Jump_Num_3;
			}
		}
		else
		{
			if (OK_CNT_2 >= OK_CNT_3)
			{
				OK_CNT = OK_CNT_2;
				Jump_Num = Jump_Num_2;
			}
			else
			{
				OK_CNT = OK_CNT_3;
				Jump_Num = Jump_Num_3;
			}
		}
	}
	hobot_write_lt8618sxb(0xff, 0x81);

	if ((Jump_CNT == 2) || (Jump_CNT == 4) || (Jump_CNT == 6))
	{
		hobot_write_lt8618sxb(0x27,
							  (0x60 +
							   (Jump_Num + (OK_CNT / 2)) % 0x0a));
	}

	if (OK_CNT == 0x0a)
	{
		hobot_write_lt8618sxb(0x27, (0x60 + (Temp_f + 5) % 0x0a));
	}
#ifdef _Phase_Debug_
	xj3_display_debug("cail phase is 0x%x", hobot_read_lt8618sxb(0x27));
#endif

	return 1;
}
#endif
#endif

static int set_pin_output_value(char pin, unsigned int val)
{
	unsigned int reg = 0;
	unsigned int offset = 0;

	if (pin <= 0 || pin > HB_PIN_MAX_NUMS)
	{
		printf("set pin is error\n");
		return 0;
	}

	/* set pin to gpio*/
	offset = pin * 4;
	reg = reg32_read(X2A_PIN_SW_BASE + offset);
	reg |= 3;
	reg32_write(X2A_PIN_SW_BASE + offset, reg);

	/* set pin to output and value*/
	offset = (pin / 16) * 0x10 + 0x08;
	reg = reg32_read(X2_GPIO_BASE + offset);
	reg |= (1 << ((pin % 16) + 16));
	if (val == 1)
	{
		reg |= (1 << (pin % 16));
	}
	else
	{
		reg &= (~(1 << (pin % 16)));
	}

	reg32_write(X2_GPIO_BASE + offset, reg);

	return 0;
}

void LT8618SXB_Reset(void)
{

	// The reset function has been moved to board/hobot/common/board.c

	// set_pin_output_value(115, 1);
	// msleep(100);
	// set_pin_output_value(115, 0);
	return;
}
void LT8618SXB_TTL_Input_Analog(void)
{
	// printf("*****LT8618SXB_TTL_Input_Analog*****\n");
	//  TTL mode
	hobot_write_lt8618sxb(0xff, 0x81); // register bank
	hobot_write_lt8618sxb(0x02, 0x66);
	hobot_write_lt8618sxb(0x0a, 0x06);
	hobot_write_lt8618sxb(0x15, 0x06);

	hobot_write_lt8618sxb(0x4e, 0xa8);

	hobot_write_lt8618sxb(0xff, 0x82);
	hobot_write_lt8618sxb(0x1b, 0x75);
	hobot_write_lt8618sxb(0x1c, 0x30); // 30000
}

static void parse_timing(struct xj3_iar_priv *priv)
{
	int i = 0;
	int aspect_ratio = 0;
	int pixel_clk_mhz = 0;
	Format_Timing[_custom_][0] = priv->display_timing.hfp;
	Format_Timing[_custom_][1] = priv->display_timing.hs;
	Format_Timing[_custom_][2] = priv->display_timing.hbp;
	Format_Timing[_custom_][3] = priv->iar_channel_base_cfg.width;
	Format_Timing[_custom_][4] = Format_Timing[_custom_][0] + Format_Timing[_custom_][1] + Format_Timing[_custom_][2] + Format_Timing[_custom_][3];
	Format_Timing[_custom_][5] = priv->display_timing.vfp;
	Format_Timing[_custom_][6] = priv->display_timing.vs;
	Format_Timing[_custom_][7] = priv->display_timing.vbp;
	Format_Timing[_custom_][8] = priv->iar_channel_base_cfg.height;
	Format_Timing[_custom_][9] = Format_Timing[_custom_][5] + Format_Timing[_custom_][6] + Format_Timing[_custom_][7] + Format_Timing[_custom_][8];
	Format_Timing[_custom_][10] = 0;

	aspect_ratio = (Format_Timing[_custom_][4] * 1000) / (Format_Timing[_custom_][9]);
	if (aspect_ratio > 1750 && aspect_ratio < 1780)
	{
		Format_Timing[18][11] = _16_9_;
	}
	else if (aspect_ratio > 1320 && aspect_ratio < 1340)
	{
		Format_Timing[18][11] = _4_3_;
	}
	else
	{
		xj3_display_debug("Unsupport ratio,using 4:3\n");
		Format_Timing[18][11] = _4_3_;
	}
	pixel_clk_mhz = priv->iar_bt1120_pix_clk / 1000000;
	xj3_display_debug("pixel clk:%d Mhz\n", pixel_clk_mhz);
	if (pixel_clk_mhz > 100)
	{
		Format_Timing[18][12] = _Greater_than_100M;
		Format_Timing[18][13] = pixel_clk_mhz / 2 >= 100 ? _Greater_than_100M : _Bound_50_100M;
		// pr_debug("Format_Timing[18][12]:%d,Format_Timing[18][13]:%d\n", Format_Timing[18][12], Format_Timing[18][13]);
	}
	else if (pixel_clk_mhz <= 100 && pixel_clk_mhz >= 50)
	{
		Format_Timing[18][12] = _Bound_50_100M;
		Format_Timing[18][13] = pixel_clk_mhz / 2 >= 50 ? _Bound_50_100M : _Less_than_50M;
		// pr_debug("Format_Timing[18][12]:%d,Format_Timing[18][13]:%d\n", Format_Timing[18][12], Format_Timing[18][13]);
	}
	else if (pixel_clk_mhz < 50)
	{
		Format_Timing[18][12] = _Less_than_50M;
		Format_Timing[18][13] = _Less_than_50M;
	}

	for (i = 0; i < 14; i++)
	{
		// xj3_display_debug("timing[_custom_][%d]:%d\n", i, Format_Timing[_custom_][i]);
	}
}
extern uint32_t hb_som_type_get(void);
static int lt8618_detect(void)
{
	uint32_t som_type = hb_som_type_get();
	int i2c_bus = 5; // CM default
	switch (som_type)
	{
	case SOM_TYPE_X3PI:
	case SOM_TYPE_X3PIV2:
	case SOM_TYPE_X3PIV2_1:
		i2c_bus = 1;
		break;
	case SOM_TYPE_X3CM:
		i2c_bus = 5;
		break;
	default:
		printf("%s :There is nothing to do,return!\n", __func__);
		break;
	}
	return i2c_bus;
}

int xj3_hdmi_probe(struct udevice *dev)
{
	xj3_display_debug("HDMI probe start!\n");
	struct xj3_iar_priv *priv = dev_get_priv(dev);
	parse_timing(priv);
	Use_DDRCLK = 0; // 1: DDR mode; 0: SDR (normal) mode
	xj3_display_debug("HDMI probe start! 00\n");

	// LT8618SXB_Reset();
	i2c_get_chip_for_busnum(i2c_bus, 0x3b, 1, &i2c_dev);
	xj3_display_debug("HDMI probe start! 01\n");

	// LT8618SXB_Reset();
	// i2c_get_chip_for_busnum(5, 0x3b, 1, &i2c_dev);
	// Parameters required by LT8618SXB_BT_Timing_setting(void)
	Resolution_Num = _custom_;
	CLK_bound =
		Format_Timing[Resolution_Num][Clk_bound_SDR + (u8)(Use_DDRCLK)];
	VIC_Num = Format_Timing[Resolution_Num][Vic];

	I2CADR = _LT8618SX_ADR; // 设置IIC地址

	//********************************************************//
	// Before initializing lt8168sxb, you need to enable IIC of lt8618sxb
	hobot_write_lt8618sxb(0xff, 0x80); // register bank
	hobot_write_lt8618sxb(0xee, 0x01); // enable IIC
	//********************************************************//

	LT8618SXB_Chip_ID(); // for debug
	LT8618SXB_RST_PD_Init();

	// TTL mode
	LT8618SXB_TTL_Input_Analog();
	LT8618SXB_TTL_Input_Digtal();

	// Wait for the signal to be stable
	// and decide whether the delay is necessary according to the actual situation
	msleep(100); // 等待信号稳定,根据实际情况决定是否需要延时
	// LT8618SXB_Video_check(); // For debug

	//------------------------PLL----------------------------------//
	LT8618SXB_PLL_setting();

	//-------------------------------------------
	LT8618SXB_Audio_setting();

	//-------------------------------------------
	LT8618SXB_CSC_setting();

	//-------------------------------------------
#ifdef _LT8618_HDCP_
	LT8618SXB_HDCP_Init();
#endif

	//-------------------------------------------
	LT8618SXB_AVI_setting();

	// This operation is not necessary. Read TV EDID if necessary.
	// LT8618SXB_Read_EDID(NULL);	// Read TV  EDID

	//-------------------------------------------
#ifdef _LT8618_HDCP_
	LT8618SXB_HDCP_Enable();
#endif

#ifdef _Embedded_sync_
	//-------------------------------------------
	LT8618SXB_BT_Timing_setting();

	if (flag_Ver_u3)
	{
		LT8618SX_Phase_1();
	}
	else
	{
		LT8618SXB_Phase_config();
	}
#else
	LT8618SX_Phase_1();
#endif

	//-------------------------------------------

	//    LT8618SXB_RST_PD_Init();

	//-------------------------------------------

	// HDMI_TX_Phy
	LT8618SXB_TX_Phy();
	xj3_display_debug("%s probe done!\n", __func__);
	return 0;
}

int  xj3_lt8618_get_edid(u8 *edid)
{
	xj3_display_debug("%s Start!\n", __func__);
	int i = 0, j = 0;
	int ret = 0;
	// u8 extended_flag = 0x00;
	xj3_display_debug("%s Start! 00\n", __func__);
	// LT8618SXB_Reset();
	// i2c_get_chip_for_busnum(5, 0x3b, 1, &i2c_dev);
	xj3_display_debug("%s Start! 01\n", __func__);

	LT8618SXB_Reset();
	i2c_bus = lt8618_detect();
	i2c_get_chip_for_busnum(i2c_bus, 0x3b, 1, &i2c_dev);

	//********************************************************//
	// Before initializing lt8168sxb, you need to enable IIC of lt8618sxb
	hobot_write_lt8618sxb(0xff, 0x80); // register bank
	hobot_write_lt8618sxb(0xee, 0x01); // enable IIC
	//********************************************************//

	hobot_write_lt8618sxb(0xff, 0x85);
	// hobot_write_lt8618sxb(0x02,0x0a); //I2C 100K
	hobot_write_lt8618sxb(0x03, 0xc9);
	hobot_write_lt8618sxb(0x04, 0xa0); // 0xA0 is EDID device address
	hobot_write_lt8618sxb(0x05, 0x00); // 0x00 is EDID offset address
	hobot_write_lt8618sxb(0x06, 0x20); // length for read
	hobot_write_lt8618sxb(0x14, 0x7f);

	for (i = 0; i < 4; i++)
	{										 // block 0 & 1
		hobot_write_lt8618sxb(0x05, i * 32); // 0x00 is EDID offset address
		hobot_write_lt8618sxb(0x07, 0x36);
		hobot_write_lt8618sxb(0x07, 0x34);
		hobot_write_lt8618sxb(0x07, 0x37);
		msleep(5); // wait 5ms for reading edid data.
		if (hobot_read_lt8618sxb(0x40) & 0x02)
		{
			// DDC No Ack or Abitration lost
			if (hobot_read_lt8618sxb(0x40) & 0x50)
			{
				printf("LT8618: read edid failed: no ack\n");
				ret = -1;
				goto end;
			}
			else
			{
				for (j = 0; j < 32; j++)
				{
					edid[i * 32 + j] = hobot_read_lt8618sxb(0x83);
				}
			}
		}
		else
		{
			printf("LT8618: read edid failed: accs not done\n");
			ret = -1;
			goto end;
		}
	}
end:
	hobot_write_lt8618sxb(0x03, 0xc2);
	hobot_write_lt8618sxb(0x07, 0x1f);
	return ret;
}
