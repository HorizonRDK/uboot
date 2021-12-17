/*
 * Horizon Robotics
 *
 *  Copyright (C) 2021 Horizon Robotics Inc.
 *  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
//#include <asm/arch/clock.h>
//#include <asm/arch/sys_proto.h>
#include <lcd.h>
#include <mmc.h>
#include <blk.h>
#include <mapmem.h>
#include <linux/iopoll.h>
#include <part.h>
#include "./hobot_lcd.h"

vidinfo_t panel_info = {
	.vl_col =		720,
	.vl_row =		1280,
	.vl_rot = 0,
	.vl_bpix = 24,
	.cmap = NULL,
	.priv = NULL,
};

#define FB_BASE 0x2E000000
#define FB_BASE_SIZE 0x7e9000 //1920*1080*4
#define CLK_BASE_REG_ADDR 0xa1000000
#define VIOSYS_CLKEN 0x140
#define VIOSYS_CLK_DIV_SEL1 0x240
#define PIN_MUX_BASE_ADDR 0xa6004000
#define RESET_PIN_OFFSET 0x70

#define GPIO_BASE_ADDR 0xa6003000
#define GPIO_1_OUTPUT_CTRL_OFFSET 0x18

#define MIPI_TX_DPHY_CTRL_ADDR 0xA43000E0
#define MIPI_DEV_FREQUENCY_ADDR 0xA430008C

const unsigned int g_mipi_dsi_reg_cfg_table[][3] = {
        /*reg mask      reg offset*/
        {0x1, 0x0},//0
        {0xff, 0x0},//1
        {0xff, 0x8},//2
        {0x1, 0x0},//3
        {0x1, 0x1},//4
        {0x3, 0x0},//5
        {0xff, 0x8},//6
        {0x1, 0x0},//7
        {0x1, 0x1},//8
        {0x1, 0x2},//9
        {0x1, 0x3},//10
        {0x3, 0x0},//11
        {0xf, 0x0},//12
        {0x3fff, 0x0},//13
        {0x1fff, 0x0},//14
        {0x1fff, 0x0},//15
        {0xfff, 0x0},//16
        {0xfff, 0x0},//17
        {0x7fff, 0x0},//18
        {0x3ff, 0x0},//19
        {0x3ff, 0x0},//20
        {0x3ff, 0x0},//21
        {0x3fff, 0x0},//22
        {0x1, 0x0},//23
        {0x3, 0x0},//24
        {0x1, 0x8},//25
        {0x1, 0x9},//26
        {0x1, 0xa},//27
        {0x1, 0xb},//28
        {0x1, 0xc},//29
        {0x1, 0xd},//30
        {0x1, 0xe},//31
        {0x1, 0xf},//32
        {0x1, 0x10},//33
        {0x1, 0x14},//34
	{0x1, 0x18},//35
        {0x1, 0x0},//36
        {0x1, 0x1},//37
        {0x1, 0x8},//38
        {0x1, 0x9},//39
        {0x1, 0xa},//40
        {0x1, 0xb},//41
        {0x1, 0xc},//42
        {0x1, 0xd},//43
        {0x1, 0xe},//44
        {0x1, 0x10},//45
        {0x1, 0x11},//46
        {0x1, 0x12},//47
        {0x1, 0x13},//48
        {0x1, 0x18},//49
        {0x3f, 0x0},//50
        {0x3, 0x6},//51
        {0xff, 0x8},//52
        {0xff, 0x10},//53
};

const unsigned int g_iarReg_cfg_table[][3] = {
        /*reg mask      reg offset*/
        {0x1, 0x1f},    /*ALPHA_SELECT_PRI4*/
        {0x1, 0x1e},    /*ALPHA_SELECT_PRI3*/
        {0x1, 0x1d},    /*ALPHA_SELECT_PRI2*/
        {0x1, 0x1c},    /*ALPHA_SELECT_PRI1*/
        {0x1, 0x1b},    /*EN_RD_CHANNEL4*/
        {0x1, 0x1a},    /*EN_RD_CHANNEL3*/
        {0x1, 0x19},    /*EN_RD_CHANNEL2*/
        {0x1, 0x18},    /*EN_RD_CHANNEL1*/
        {0x3, 0x16},    /*LAYER_PRIORITY_4*/
        {0x3, 0x14},    /*LAYER_PRIORITY_3*/
        {0x3, 0x12},    /*LAYER_PRIORITY_2*/
        {0x3, 0x10},    /*LAYER_PRIORITY_1*/
        {0x1, 0xf},     /*EN_OVERLAY_PRI4*/
        {0x1, 0xe},     /*EN_OVERLAY_PRI3*/
        {0x1, 0xd},     /*EN_OVERLAY_PRI2*/
        {0x1, 0xc},     /*EN_OVERLAY_PRI1*/
        {0x3, 0xa},     /*OV_MODE_PRI4*/
        {0x3, 0x8},     /*OV_MODE_PRI3*/
        {0x3, 0x6},     /*OV_MODE_PRI2*/
        {0x3, 0x4},     /*OV_MODE_PRI1*/
        {0x1, 0x3},     /*EN_ALPHA_PRI4*/
        {0x1, 0x2},     /*EN_ALPHA_PRI3*/
        {0x1, 0x1},     /*EN_ALPHA_PRI2*/
        {0x1, 0x0},     /*EN_ALPHA_PRI1*/
        /*X2_IAR_ALPHA_VALUE*/
        {0xff, 0x18},   /*ALPHA_RD4*/
        {0xff, 0x10},   /*ALPHA_RD3*/
        {0xff, 0x8},    /*ALPHA_RD2*/
        {0xff, 0x0},    /*ALPHA_RD1*/
        /*X2_IAR_CROPPED_WINDOW_RD_x*/
        {0x7ff, 0x10},  /*WINDOW_HEIGTH*/
        {0x7ff, 0x0},   /*WINDOW_WIDTH*/
        /*X2_IAR_WINDOW_POSITION_FBUF_RD_x*/
        {0xfff, 0x10},  /*WINDOW_START_Y*/
        {0xfff, 0x0},   /*WINDOW_START_X*/
	/*X2_IAR_FORMAT_ORGANIZATION*/
        {0x1, 0xf},     /*BT601_709_SEL*/
        {0x1, 0xe},     /*RGB565_CONVERT_SEL*/
        {0x7, 0xb},     /*IMAGE_FORMAT_ORG_RD4*/
        {0x7, 0x8},     /*IMAGE_FORMAT_ORG_RD3*/
        {0xf, 0x4},     /*IMAGE_FORMAT_ORG_RD2*/
        {0xf, 0x0},     /*IMAGE_FORMAT_ORG_RD1*/
        /*X2_IAR_DISPLAY_POSTION_RD_x*/
        {0x7ff, 0x10},  /*LAYER_TOP_Y*/
        {0x7ff, 0x0},   /*LAYER_LEFT_X*/
        /*X2_IAR_HWC_CFG*/
        {0xffffff, 0x2},    /*HWC_COLOR*/
        {0x1, 0x1},     /*HWC_COLOR_EN*/
        {0x1, 0x0},     /*HWC_EN*/
        /*X2_IAR_HWC_SIZE*/
        {0x7ff, 0x10},  /*HWC_HEIGHT*/
        {0x7ff, 0x0},   /*HWC_WIDTH*/
        /*X2_IAR_HWC_POS*/
        {0x7ff, 0x10},  /*HWC_TOP_Y*/
        {0x7ff, 0x0},   /*HWC_LEFT_X*/
        /*X2_IAR_BG_COLOR*/
        {0xffffff, 0x0},    /*BG_COLOR*/
        /*X2_IAR_SRC_SIZE_UP*/
        {0x7ff, 0x10},  /*SRC_HEIGTH*/
        {0x7ff, 0x0},   /*SRC_WIDTH*/
        /*X2_IAR_TGT_SIZE_UP*/
        {0x7ff, 0x10},  /*TGT_HEIGTH*/
        {0x7ff, 0x0},   /*TGT_WIDTH*/
        /*X2_IAR_STEP_UP*/
        {0xfff, 0x10},  /*STEP_Y*/
        {0xfff, 0x0},   /*STEP_X*/
        /*X2_IAR_UP_IMAGE_POSTION*/
        {0x7ff, 0x10},  /*UP_IMAGE_TOP_Y*/
        {0x7ff, 0x0},   /*UP_IMAGE_LEFT_X*/
        /*X2_IAR_PP_CON_1*/
        {0x3f, 0xa},    /*CONTRAST*/
	{0x1, 0x9},     /*THETA_SIGN*/
        {0x1, 0x8},     /*UP_SCALING_EN*/
        {0x1, 0x7},     /*ALGORITHM_SELECT*/
        {0x1, 0x6},     /*BRIGHT_EN*/
        {0x1, 0x5},     /*CON_EN*/
        {0x1, 0x4},     /*SAT_EN*/
        {0x1, 0x3},     /*HUE_EN*/
        {0x1, 0x2},     /*GAMMA_ENABLE*/
        {0x1, 0x1},     /*DITHERING_EN*/
        {0x1, 0x0},     /*DITHERING_FLAG*/
        /*X2_IAR_PP_CON_2*/
        {0xff, 0x18},   /*OFF_BRIGHT*/
        {0xff, 0x10},   /*OFF_CONTRAST*/
        {0xff, 0x8},    /*SATURATION*/
        {0xff, 0x0},    /*THETA_ABS*/
        /*X2_IAR_THRESHOLD_RD4_3*/
        {0x7ff, 0x10},  /*THRESHOLD_RD4*/
        {0x7ff, 0x0},   /*THRESHOLD_RD3*/
        /*X2_IAR_THRESHOLD_RD2_1*/
        {0x7ff, 0x10},  /*THRESHOLD_RD2*/
        {0x7ff, 0x0},   /*THRESHOLD_RD1*/
        /*X2_IAR_CAPTURE_CON*/
        {0x1, 0x6},     /*CAPTURE_INTERLACE*/
        {0x1, 0x5},     /*CAPTURE_MODE*/
        {0x3, 0x3},     /*SOURCE_SEL*/
        {0x7, 0x0},     /*OUTPUT_FORMAT*/
        /*X2_IAR_CAPTURE_SLICE_LINES*/
        {0xfff, 0x0},   /*SLICE_LINES*/
        /*X2_IAR_BURST_LEN*/
        {0xf, 0x4},     /*BURST_LEN_WR*/
        {0xf, 0x0},     /*BURST_LEN_RD*/
        /*X2_IAR_UPDATE*/
        {0x1, 0x0},     /*UPDATE*/
        /*X2_IAR_PANEL_SIZE*/
        {0x7ff, 0x10},  /*PANEL_HEIGHT*/
        {0x7ff, 0x0},   /*PANEL_WIDTH*/
	/*X2_IAR_REFRESH_CFG*/
        {0x1, 0x11},    /*ITU_R_656_EN*/
        {0x1, 0x10},    /*UV_SEQUENCE*/
        {0xf, 0xc},     /*P3_P2_P1_P0*/
        {0x1, 0xb},     /*YCBCR_OUTPUT*/
        {0x3, 0x9},     /*PIXEL_RATE*/
        {0x1, 0x8},     /*ODD_POLARITY*/
        {0x1, 0x7},     /*DEN_POLARITY*/
        {0x1, 0x6},     /*VSYNC_POLARITY*/
        {0x1, 0x5},     /*HSYNC_POLARITY*/
        {0x1, 0x4},     /*INTERLACE_SEL*/
        {0x3, 0x2},     /*PANEL_COLOR_TYPE*/
        {0x1, 0x1},     /*DBI_REFRESH_MODE*/
        /*X2_IAR_PARAMETER_HTIM_FIELD1*/
        {0x3ff, 0x14},  /*DPI_HBP_FIELD*/
        {0x3ff, 0xa},   /*DPI_HFP_FIELD*/
        {0x3ff, 0x0},   /*DPI_HSW_FIELD*/
        /*X2_IAR_PARAMETER_VTIM_FIELD1*/
        {0x3ff, 0x14},  /*DPI_VBP_FIELD*/
        {0x3ff, 0xa},   /*DPI_VFP_FIELD*/
        {0x3ff, 0x0},   /*DPI_VSW_FIELD*/
        /*X2_IAR_PARAMETER_HTIM_FIELD2*/
        {0x3ff, 0x14},  /*DPI_HBP_FIELD2*/
        {0x3ff, 0xa},   /*DPI_HFP_FIELD2*/
        {0x3ff, 0x0},   /*DPI_HSW_FIELD2*/
        /*X2_IAR_PARAMETER_VTIM_FIELD2*/
        {0x3ff, 0x14},  /*DPI_VBP_FIELD2*/
        {0x3ff, 0xa},   /*DPI_VFP_FIELD2*/
        {0x3ff, 0x0},   /*DPI_VSW_FIELD2*/
        /*X2_IAR_PARAMETER_VFP_CNT_FIELD12*/
        {0xffff, 0x0},  /*PARAMETER_VFP_CNT*/
        /*X2_IAR_GAMMA_X1_X4_R*/
        {0xff, 0x18},   /*GAMMA_XY_D_R*/
        {0xff, 0x10},   /*GAMMA_XY_C_R*/
        {0xff, 0x8},    /*GAMMA_XY_B_R*/
        {0xff, 0x0},    /*GAMMA_XY_A_R*/
	/*X2_IAR_GAMMA_Y16_RGB*/
        {0xff, 0x10},   /*GAMMA_Y16_R*/
        {0xff, 0x8},    /*GAMMA_Y16_G*/
        {0xff, 0x0},    /*GAMMA_Y16_B*/
        /*X2_IAR_HWC_SRAM_WR*/
        {0x1ff, 0x10},  /*HWC_SRAM_ADDR*/
        {0xffff, 0x0},  /*HWC_SRAM_D*/
        /*X2_IAR_PALETTE*/
        {0xff, 0x18},   /*PALETTE_INDEX*/
        {0xffffff, 0x0},    /*PALETTE_DATA*/
        /*X2_IAR_DE_SRCPNDREG*/
        {0x1, 0x1a},    /*INT_WR_FIFO_FULL*/
        {0x1, 0x19},    /*INT_CBUF_SLICE_START*/
        {0x1, 0x18},    /*INT_CBUF_SLICE_END*/
        {0x1, 0x17},    /*INT_CBUF_FRAME_END*/
        {0x1, 0x16},    /*INT_FBUF_FRAME_END*/
        {0x1, 0x15},    /*INT_FBUF_SWITCH_RD4*/
        {0x1, 0x14},    /*INT_FBUF_SWITCH_RD3*/
        {0x1, 0x13},    /*INT_FBUF_SWITCH_RD2*/
        {0x1, 0x12},    /*INT_FBUF_SWITCH_RD1*/
        {0x1, 0x11},    /*INT_FBUF_START_RD4*/
        {0x1, 0x10},    /*INT_FBUF_START_RD3*/
        {0x1, 0xf},     /*INT_FBUF_START_RD2*/
        {0x1, 0xe},     /*INT_FBUF_START_RD1*/
        {0x1, 0xd},     /*INT_FBUF_START*/
        {0x1, 0xc},     /*INT_BUFFER_EMPTY_RD4*/
        {0x1, 0xb},     /*INT_BUFFER_EMPTY_RD3*/
        {0x1, 0xa},     /*INT_BUFFER_EMPTY_RD2*/
        {0x1, 0x9},     /*INT_BUFFER_EMPTY_RD1*/
        {0x1, 0x8},     /*INT_THRESHOLD_RD4*/
        {0x1, 0x7},     /*INT_THRESHOLD_RD3*/
        {0x1, 0x6},     /*INT_THRESHOLD_RD2*/
        {0x1, 0x5},     /*INT_THRESHOLD_RD1*/
        {0x1, 0x4},     /*INT_FBUF_END_RD4*/
        {0x1, 0x3},     /*INT_FBUF_END_RD3*/
        {0x1, 0x2},     /*INT_FBUF_END_RD2*/
	{0x1, 0x1},     /*INT_FBUF_END_RD1*/
        {0x1, 0x0},     /*INT_VSYNC*/
        /*X2_IAR_DE_REFRESH_EN*/
        {0x1, 0x1},     /*AUTO_DBI_REFRESH_EN*/
        {0x1, 0x0},     /*DPI_TV_START*/
        /*X2_IAR_DE_CONTROL_WO*/
        {0x1, 0x2},     /*FIELD_ODD_CLEAR*/
        {0x1, 0x1},     /*DBI_START*/
        {0x1, 0x0},     /*CAPTURE_EN*/
        /*X2_IAR_DE_STATUS*/
        {0xf, 0x1c},    /*DMA_STATE_RD4*/
        {0xf, 0x18},    /*DMA_STATE_RD3*/
        {0xf, 0x14},    /*DMA_STATE_RD2*/
        {0xf, 0x10},    /*DMA_STATE_RD1*/
        {0xf, 0xc},     /*DMA_STATE_WR*/
        {0x7, 0x9},     /*DPI_STATE*/
        {0xf, 0x5},     /*DBI_STATE*/
        {0x1, 0x1},     /*CAPTURE_STATUS*/
        {0x1, 0x0},     /*REFRESH_STATUS*/
        /*X2_IAR_AXI_DEBUG_STATUS1*/
        {0xffff, 0x3},  /*CUR_BUF*/
        {0x1, 0x2},     /*CUR_BUFGRP*/
        {0x1, 0x1},     /*STALL_OCCUR*/
        {0x1, 0x0},     /*ERROR_OCCUR*/
        /*X2_IAR_DE_MAXOSNUM_RD*/
        {0x7, 0x1c},    /*MAXOSNUM_DMA0_RD1*/
        {0x7, 0x18},    /*MAXOSNUM_DMA1_RD1*/
        {0x7, 0x14},    /*MAXOSNUM_DMA2_RD1*/
        {0x7, 0x10},    /*MAXOSNUM_DMA0_RD2*/
        {0x7, 0xc},     /*MAXOSNUM_DMA1_RD2*/
        {0x7, 0x8},     /*MAXOSNUM_DMA2_RD2*/
        {0x7, 0x4},     /*MAXOSNUM_RD3*/
        {0x7, 0x0},     /*MAXOSNUM_RD4*/
        /*X2_IAR_DE_MAXOSNUM_WR*/
        {0x7, 0x4}, /*MAXOSNUM_DMA0_WR*/
        {0x7, 0x0}, /*MAXOSNUM_DMA1_WR*/
	/*X2_IAR_DE_SW_RST*/
        {0x1, 0x1},     /*WR_RST*/
        {0x1, 0x0},         /*RD_RST*/
        /*X2_IAR_DE_OUTPUT_SEL*/
        {0x1, 0x3},     /*IAR_OUTPUT_EN*/
        {0x1, 0x2},     /*RGB_OUTPUT_EN*/
        {0x1, 0x1},     /*BT1120_OUTPUT_EN*/
        {0x1, 0x0},     /*MIPI_OUTPUT_EN*/
        /*X2_IAR_DE_AR_CLASS_WEIGHT*/
        {0xffff, 0x10}, /*AR_CLASS3_WEIGHT*/
        {0xffff, 0x0},   /*AR_CLASS2_WEIGHT*/
};

#define REG_MIPI_DSI_PHY_TST_CTRL0  0xb4
#define REG_MIPI_DSI_PHY_TST_CTRL1  0xb8

#define DPHY_TEST_ENABLE    0x10000
#define DPHY_TEST_CLK       0x2
#define DPHY_TEST_RESETN    0x0

#define VALUE_SET(value, mask, offset, regvalue)        \
        ((((value) & (mask)) << (offset)) | ((regvalue)&~((mask) << (offset))))
#define VALUE_GET(mask, offset, regvalue) (((regvalue) >> (offset)) & (mask))

#define MIPI_DSI_REG_SET_FILED(key, value, regvalue)    \
        VALUE_SET(value, g_mipi_dsi_reg_cfg_table[key][TABLE_MASK], \
                g_mipi_dsi_reg_cfg_table[key][TABLE_OFFSET], regvalue)
#define MIPI_DSI_REG_GET_FILED(key, regvalue)   \
        VALUE_GET(g_mipi_dsi_reg_cfg_table[key][TABLE_MASK], \
                g_mipi_dsi_reg_cfg_table[key][TABLE_OFFSET], regvalue)

#define FBUF_SIZE_ADDR_OFFSET(X)  (REG_IAR_CROPPED_WINDOW_RD1-((X)*0x4))
#define FBUF_WIDTH_ADDR_OFFSET(X)  (REG_IAR_IMAGE_WIDTH_FBUF_RD1-((X)*0x4))
#define WIN_POS_ADDR_OFFSET(X)  (REG_IAR_DISPLAY_POSTION_RD1-((X)*0x4))
#define KEY_COLOR_ADDR_OFFSET(X)  (REG_IAR_KEY_COLOR_RD1-((X)*0x4))

#define IAR_REG_SET_FILED(key, value, regvalue) \
	VALUE_SET(value, g_iarReg_cfg_table[key][TABLE_MASK], \
	g_iarReg_cfg_table[key][TABLE_OFFSET], regvalue)
#define IAR_REG_GET_FILED(key, regvalue) \
	VALUE_GET(g_iarReg_cfg_table[key][TABLE_MASK], \
	g_iarReg_cfg_table[key][TABLE_OFFSET], regvalue)

struct video_timing video_1080_1920 = {
        1080, 0, 0, 16, 77, 1296, 4, 4, 100, 1920,
};
struct video_timing video_720_1280 = {
        720, 0, 0, 10, 30, 900, 10, 20, 10, 1320,
};

struct video_timing video_720_1280_sdb = {
        720, 0, 0, 10, 40, 810, 3, 11, 16, 1310,
};

struct video_timing video_1280_720 = {
        1280, 0, 0, 10, 40, 1370, 3, 11, 16, 750,
};

struct disp_timing video_720x1280_touch = {
        100, 100, 10, 20, 10, 10, 10
};

static void mipi_dphy_write(uint16_t addr, uint8_t data)
{
        uint32_t regv = 0;

        /*write test code*/
        /*set testen to high*/
        writel(DPHY_TEST_ENABLE,
                        MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testclk to high*/
        writel(DPHY_TEST_CLK,
                        MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
        /*set testclk to low*/
        writel(DPHY_TEST_RESETN,
                        MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
        /*set testen to low, set test code MSBS*/
        writel(addr >> 8, MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testclk to high*/
        writel(DPHY_TEST_CLK,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);

        /*set testclk to low*/
        writel(DPHY_TEST_RESETN,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
        regv = readl(MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testen to high*/
        writel(DPHY_TEST_ENABLE | regv,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testclk to high*/
        writel(DPHY_TEST_CLK,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
        /*set test code LSBS*/
        writel(DPHY_TEST_ENABLE | (addr & 0xff),
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testclk to low*/
        writel(DPHY_TEST_RESETN,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);

        /*set test data*/
        writel(data, MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL1);
        /*set testclk to high*/
        writel(DPHY_TEST_CLK,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
        /*set testclk to low*/
        writel(DPHY_TEST_RESETN,
                MIPI_DSI_CORE_BASE_ADDR + REG_MIPI_DSI_PHY_TST_CTRL0);
}

static void mipi_dphy_config(uint8_t panel_no)
{
        if (panel_no == 0) {
                mipi_dphy_write(0x270, 0x5e);
                mipi_dphy_write(0x272, 0x11);
                mipi_dphy_write(0x179, 0xda);
                mipi_dphy_write(0x17a, 0x0);
                mipi_dphy_write(0x178, 0xda);
                mipi_dphy_write(0x17b, 0xbf);
                mipi_dphy_write(0x65, 0x80);
                mipi_dphy_write(0x15e, 0x10);
                mipi_dphy_write(0x162, 0x4);
                mipi_dphy_write(0x16e, 0xc);
                mipi_dphy_write(0x170, 0xff);
                mipi_dphy_write(0x160, 0x6);
                mipi_dphy_write(0x161, 0x1);
                mipi_dphy_write(0x1ac, 0x10);
                mipi_dphy_write(0x1d, 0x4);
                mipi_dphy_write(0x8, 0x3);
                mipi_dphy_write(0x72, 0x11);
                mipi_dphy_write(0x1b, 0xaa);
                mipi_dphy_write(0x1c, 0xaa);
        } else if (panel_no == 1 || panel_no == 2) {
                mipi_dphy_write(0x1d, 0x4);
                mipi_dphy_write(0x1c, 0xaa);
        }
}

static int mipi_dsi_core_reset(void)
{
        uint32_t value;

        value = MIPI_DSI_REG_SET_FILED(SHUTDOWNZ_FILED, 0x0, 0x0);
        writel(value, MIPI_DSI_CORE_BASE_ADDR + PWR_UP);//0x4
        return 0;
}

static int mipi_dsi_core_start(void)
{
        uint32_t value;

        value = MIPI_DSI_REG_SET_FILED(SHUTDOWNZ_FILED, 0x1, 0x0);
        writel(value, MIPI_DSI_CORE_BASE_ADDR + PWR_UP);//0x4
        return 0;
}

static int mipi_dsi_core_pre_init(uint8_t panel_no)
{
        uint32_t reg_val;

	writel(0x1, MIPI_TX_DPHY_CTRL_ADDR);
	reg_val = readl(MIPI_DEV_FREQUENCY_ADDR);
	if (panel_no == 0) {
		reg_val = (reg_val & 0xffffff80) | 0x00000023;
	} else if (panel_no == 1 || panel_no == 2) {
		reg_val = (reg_val & 0xffff0000) | 0x00001c16;
	}
	writel(0x08100000, 0xa4300084);
	writel(reg_val, MIPI_DEV_FREQUENCY_ADDR);

        writel(0x0, MIPI_DSI_CORE_BASE_ADDR + PHY_RSTZ);//0xa0
        mipi_dsi_core_reset();
        writel(0x0, MIPI_DSI_CORE_BASE_ADDR + MODE_CFG);//0X34
        writel(0x1, MIPI_DSI_CORE_BASE_ADDR + PHY_TST_CTRL0);//0xb4
	udelay(1000);
        writel(0x0, MIPI_DSI_CORE_BASE_ADDR + PHY_TST_CTRL0);//0xb4
        writel(0x3203, MIPI_DSI_CORE_BASE_ADDR + PHY_IF_CFG);//0XA4
        writel(0x2, MIPI_DSI_CORE_BASE_ADDR + CLKMGR_CFG);//0X8
        writel(0x3, MIPI_DSI_CORE_BASE_ADDR + LPCLK_CTRL);//0X94
	udelay(1000);
        writel(0x0, MIPI_DSI_CORE_BASE_ADDR + PHY_TST_CTRL0);//0XB4
        mipi_dphy_config(panel_no);

        writel(0x7, MIPI_DSI_CORE_BASE_ADDR + PHY_RSTZ);//0XA0
        mipi_dsi_core_start();

        return 0;
}


static int mipi_dsi_dpi_config(uint8_t panel_no)
{
        uint32_t value;

        value = readl(MIPI_DSI_CORE_BASE_ADDR + DPI_VCID);
        value = MIPI_DSI_REG_SET_FILED(DPI_VCID_FILED, 0x0, value);
        writel(value, MIPI_DSI_CORE_BASE_ADDR + DPI_VCID);//0xc

        value = readl(MIPI_DSI_CORE_BASE_ADDR + DPI_COLOR_CODING);
        value = MIPI_DSI_REG_SET_FILED(DPI_COLOR_CODING_FILED, 0x5, value);
        writel(value, MIPI_DSI_CORE_BASE_ADDR + DPI_COLOR_CODING);//0x10
        return 0;
}

int mipi_dsi_set_mode(uint8_t mode)
{
        uint32_t value;

        value = readl(MIPI_DSI_CORE_BASE_ADDR + MODE_CFG);//0x34
        value = MIPI_DSI_REG_SET_FILED(CMD_VIDEO_MODE_FILED, mode, value);
        writel(value, MIPI_DSI_CORE_BASE_ADDR + MODE_CFG);
        return 0;
}

static int mipi_dsi_vid_mode_cfg(uint8_t mode)
{
        uint32_t value;

        //burst mode
        value = MIPI_DSI_REG_SET_FILED(VID_MODE_TYPE_FILED, 0x3, 0x0);
        value = MIPI_DSI_REG_SET_FILED(LP_VSA_EN_FILED, 0x1, value);
        value = MIPI_DSI_REG_SET_FILED(LP_VBP_EN_FILED, 0x1, value);
        value = MIPI_DSI_REG_SET_FILED(LP_VFP_EN_FILED, 0x1, value);
        value = MIPI_DSI_REG_SET_FILED(LP_VACT_EN_FILED, 0x1, value);
        value = MIPI_DSI_REG_SET_FILED(LP_HBP_EN_FILED, 0x1, value);
        value = MIPI_DSI_REG_SET_FILED(LP_HFP_EN_FILED, 0x1, value);

        value = MIPI_DSI_REG_SET_FILED(VPG_EN_FILED, mode, value);
        //0:normal 1:pattern

        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_MODE_CFG);//38
        return 0;
}

static int mipi_dsi_video_config(struct video_timing *video_timing_config)
{
        uint32_t value;

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_PKT_SIZE);
        value = MIPI_DSI_REG_SET_FILED(VID_PKT_SIZE_FILED,
                        video_timing_config->vid_pkt_size, value);//1080
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_PKT_SIZE);//0x3c

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_NUM_CHUNKS);
        value = MIPI_DSI_REG_SET_FILED(VID_NUM_CHUNKS_FILED,
                        video_timing_config->vid_num_chunks, value);//0
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_NUM_CHUNKS);//0x40

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_NULL_SIZE);
        value = MIPI_DSI_REG_SET_FILED(VID_NULL_SIZE_FILED,
                        video_timing_config->vid_null_size, value);//0
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_NULL_SIZE);//0x44

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_HSA_TIME);
        value = MIPI_DSI_REG_SET_FILED(VID_HSA_TIME_FILED,
                        video_timing_config->vid_hsa, value);//16
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_HSA_TIME);//0x48

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_HBP_TIME);
        value = MIPI_DSI_REG_SET_FILED(VID_HBP_TIME_FILED,
                        video_timing_config->vid_hbp, value);//512
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_HBP_TIME);//0x4c

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_HLINE_TIME);
        value = MIPI_DSI_REG_SET_FILED(VID_HLINE_TIME_FILED,
                        video_timing_config->vid_hline_time, value);//1736
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_HLINE_TIME);//0x50

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_VSA_LINES);
	value = MIPI_DSI_REG_SET_FILED(VSA_LINES_FILED,
                        video_timing_config->vid_vsa, value);//4
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_VSA_LINES);//0x54

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_VBP_LINES);
        value = MIPI_DSI_REG_SET_FILED(VBP_LINES_FILED,
                        video_timing_config->vid_vbp, value);//4
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_VBP_LINES);//0x58

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_VFP_LINES);
        value = MIPI_DSI_REG_SET_FILED(VFP_LINES_FILED,
                        video_timing_config->vid_vfp, value);//100
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_VFP_LINES);//0x5c

        value = readl(MIPI_DSI_CORE_BASE_ADDR + VID_VACTIVE_LINES);
        value = MIPI_DSI_REG_SET_FILED(V_ACTIVE_LINES_FILED,
                        video_timing_config->vid_vactive_line, value);//1920
        writel(value, MIPI_DSI_CORE_BASE_ADDR + VID_VACTIVE_LINES);//0x60

        mipi_dsi_set_mode(0);//video mode
        mipi_dsi_vid_mode_cfg(0);//normal mode
        return 0;
}

#define MIPI_SNP        0x05
#define MIPI_S1P        0x15
#define MIPI_LCP        0x39

#define MIPI_SLP        0xFE
#define MIPI_OFF        0xFF

struct mipi_init_para {
        uint8_t cmd_type;
        uint8_t cmd_len;
        uint8_t cmd[128];
};

static struct mipi_init_para init_para_720x1280_sdb[] = {
        {MIPI_LCP, 0x04, {0xff, 0x98, 0x81, 0x03}},
        {MIPI_S1P, 0x02, {0x01, 0x00}},
        {MIPI_S1P, 0x02, {0x02, 0x00}},
        {MIPI_S1P, 0x02, {0x03, 0x72}},
        {MIPI_S1P, 0x02, {0x04, 0x00}},
        {MIPI_S1P, 0x02, {0x05, 0x00}},
        {MIPI_S1P, 0x02, {0x06, 0x00}},
        {MIPI_S1P, 0x02, {0x07, 0x00}},
        {MIPI_S1P, 0x02, {0x08, 0x00}},
        {MIPI_S1P, 0x02, {0x09, 0x01}},
        {MIPI_S1P, 0x02, {0x0a, 0x00}},
        {MIPI_S1P, 0x02, {0x0b, 0x00}},
        {MIPI_S1P, 0x02, {0x0c, 0x01}},
        {MIPI_S1P, 0x02, {0x0d, 0x00}},
        {MIPI_S1P, 0x02, {0x0e, 0x00}},
        {MIPI_S1P, 0x02, {0x0f, 0x00}},
        {MIPI_S1P, 0x02, {0x10, 0x00}},
        {MIPI_S1P, 0x02, {0x11, 0x00}},
        {MIPI_S1P, 0x02, {0x12, 0x00}},
        {MIPI_S1P, 0x02, {0x13, 0x00}},
        {MIPI_S1P, 0x02, {0x14, 0x00}},
        {MIPI_S1P, 0x02, {0x15, 0x00}},
        {MIPI_S1P, 0x02, {0x16, 0x00}},
        {MIPI_S1P, 0x02, {0x17, 0x00}},
        {MIPI_S1P, 0x02, {0x18, 0x00}},
        {MIPI_S1P, 0x02, {0x19, 0x00}},
        {MIPI_S1P, 0x02, {0x1a, 0x00}},
        {MIPI_S1P, 0x02, {0x1b, 0x00}},
        {MIPI_S1P, 0x02, {0x1c, 0x00}},
        {MIPI_S1P, 0x02, {0x1d, 0x00}},
        {MIPI_S1P, 0x02, {0x1e, 0x40}},
        {MIPI_S1P, 0x02, {0x1f, 0x80}},
        {MIPI_S1P, 0x02, {0x20, 0x05}},
        {MIPI_S1P, 0x02, {0x20, 0x05}},
        {MIPI_S1P, 0x02, {0x21, 0x02}},
        {MIPI_S1P, 0x02, {0x22, 0x00}},
	{MIPI_S1P, 0x02, {0x23, 0x00}},
        {MIPI_S1P, 0x02, {0x24, 0x00}},
        {MIPI_S1P, 0x02, {0x25, 0x00}},
        {MIPI_S1P, 0x02, {0x26, 0x00}},
        {MIPI_S1P, 0x02, {0x27, 0x00}},
        {MIPI_S1P, 0x02, {0x28, 0x33}},
        {MIPI_S1P, 0x02, {0x29, 0x02}},
        {MIPI_S1P, 0x02, {0x2a, 0x00}},
        {MIPI_S1P, 0x02, {0x2b, 0x00}},
        {MIPI_S1P, 0x02, {0x2c, 0x00}},
        {MIPI_S1P, 0x02, {0x2d, 0x00}},
        {MIPI_S1P, 0x02, {0x2e, 0x00}},
        {MIPI_S1P, 0x02, {0x2f, 0x00}},
        {MIPI_S1P, 0x02, {0x30, 0x00}},
        {MIPI_S1P, 0x02, {0x31, 0x00}},
        {MIPI_S1P, 0x02, {0x32, 0x00}},
        {MIPI_S1P, 0x02, {0x32, 0x00}},
        {MIPI_S1P, 0x02, {0x33, 0x00}},
        {MIPI_S1P, 0x02, {0x34, 0x04}},
        {MIPI_S1P, 0x02, {0x35, 0x00}},
        {MIPI_S1P, 0x02, {0x36, 0x00}},
        {MIPI_S1P, 0x02, {0x37, 0x00}},
        {MIPI_S1P, 0x02, {0x38, 0x3c}},
        {MIPI_S1P, 0x02, {0x39, 0x00}},
        {MIPI_S1P, 0x02, {0x3a, 0x40}},
        {MIPI_S1P, 0x02, {0x3b, 0x40}},
        {MIPI_S1P, 0x02, {0x3c, 0x00}},
        {MIPI_S1P, 0x02, {0x3d, 0x00}},
        {MIPI_S1P, 0x02, {0x3e, 0x00}},
        {MIPI_S1P, 0x02, {0x3f, 0x00}},
        {MIPI_S1P, 0x02, {0x40, 0x00}},
        {MIPI_S1P, 0x02, {0x41, 0x00}},
        {MIPI_S1P, 0x02, {0x42, 0x00}},
        {MIPI_S1P, 0x02, {0x43, 0x00}},
	{MIPI_S1P, 0x02, {0x44, 0x00}},
        {MIPI_S1P, 0x02, {0x50, 0x01}},
        {MIPI_S1P, 0x02, {0x51, 0x23}},
        {MIPI_S1P, 0x02, {0x52, 0x45}},
        {MIPI_S1P, 0x02, {0x53, 0x67}},
        {MIPI_S1P, 0x02, {0x54, 0x89}},
        {MIPI_S1P, 0x02, {0x55, 0xab}},
        {MIPI_S1P, 0x02, {0x56, 0x01}},
        {MIPI_S1P, 0x02, {0x57, 0x23}},
        {MIPI_S1P, 0x02, {0x58, 0x45}},
        {MIPI_S1P, 0x02, {0x59, 0x67}},
        {MIPI_S1P, 0x02, {0x5a, 0x89}},
        {MIPI_S1P, 0x02, {0x5b, 0xab}},
        {MIPI_S1P, 0x02, {0x5c, 0xcd}},
        {MIPI_S1P, 0x02, {0x5d, 0xef}},
        {MIPI_S1P, 0x02, {0x5e, 0x11}},
        {MIPI_S1P, 0x02, {0x5f, 0x01}},
        {MIPI_S1P, 0x02, {0x60, 0x00}},
        {MIPI_S1P, 0x02, {0x61, 0x15}},
        {MIPI_S1P, 0x02, {0x62, 0x14}},
        {MIPI_S1P, 0x02, {0x63, 0x0e}},
        {MIPI_S1P, 0x02, {0x64, 0x0f}},
        {MIPI_S1P, 0x02, {0x65, 0x0c}},
        {MIPI_S1P, 0x02, {0x66, 0x0d}},
        {MIPI_S1P, 0x02, {0x67, 0x06}},
        {MIPI_S1P, 0x02, {0x68, 0x02}},
        {MIPI_S1P, 0x02, {0x69, 0x07}},
        {MIPI_S1P, 0x02, {0x6a, 0x02}},
        {MIPI_S1P, 0x02, {0x6b, 0x02}},
        {MIPI_S1P, 0x02, {0x6c, 0x02}},
        {MIPI_S1P, 0x02, {0x6d, 0x02}},
        {MIPI_S1P, 0x02, {0x6e, 0x02}},
        {MIPI_S1P, 0x02, {0x6f, 0x02}},
        {MIPI_S1P, 0x02, {0x70, 0x02}},
        {MIPI_S1P, 0x02, {0x71, 0x02}},
        {MIPI_S1P, 0x02, {0x72, 0x02}},
	{MIPI_S1P, 0x02, {0x73, 0x02}},
        {MIPI_S1P, 0x02, {0x74, 0x02}},
        {MIPI_S1P, 0x02, {0x75, 0x01}},
        {MIPI_S1P, 0x02, {0x76, 0x00}},
        {MIPI_S1P, 0x02, {0x77, 0x14}},
        {MIPI_S1P, 0x02, {0x78, 0x15}},
        {MIPI_S1P, 0x02, {0x79, 0x0e}},
        {MIPI_S1P, 0x02, {0x7a, 0x0f}},
        {MIPI_S1P, 0x02, {0x7b, 0x0c}},
        {MIPI_S1P, 0x02, {0x7c, 0x0d}},
        {MIPI_S1P, 0x02, {0x7e, 0x02}},
        {MIPI_S1P, 0x02, {0x7f, 0x07}},
        {MIPI_S1P, 0x02, {0x80, 0x02}},
        {MIPI_S1P, 0x02, {0x81, 0x02}},
        {MIPI_S1P, 0x02, {0x83, 0x02}},
        {MIPI_S1P, 0x02, {0x84, 0x02}},
        {MIPI_S1P, 0x02, {0x85, 0x02}},
        {MIPI_S1P, 0x02, {0x86, 0x02}},
        {MIPI_S1P, 0x02, {0x87, 0x02}},
        {MIPI_S1P, 0x02, {0x88, 0x02}},
        {MIPI_S1P, 0x02, {0x89, 0x02}},
        {MIPI_S1P, 0x02, {0x8a, 0x02}},
        {MIPI_LCP, 0x04, {0xff, 0x98, 0x81, 0x04}},
        {MIPI_S1P, 0x02, {0x6c, 0x15}},
        {MIPI_S1P, 0x02, {0x64, 0x2a}},
        {MIPI_S1P, 0x02, {0x6f, 0x34}},
        {MIPI_S1P, 0x02, {0x3a, 0x94}},
        {MIPI_S1P, 0x02, {0x8d, 0x15}},
        {MIPI_S1P, 0x02, {0x87, 0xba}},
        {MIPI_S1P, 0x02, {0x26, 0x76}},
        {MIPI_S1P, 0x02, {0xb2, 0xd1}},
        {MIPI_S1P, 0x02, {0xb5, 0x06}},
        {MIPI_LCP, 0x04, {0xff, 0x98, 0x81, 0x01}},
        {MIPI_S1P, 0x02, {0x22, 0x0a}},
        {MIPI_S1P, 0x02, {0x31, 0x00}},
        {MIPI_S1P, 0x02, {0x53, 0x90}},
	{MIPI_S1P, 0x02, {0x55, 0xa2}},
        {MIPI_S1P, 0x02, {0x50, 0xb7}},
        {MIPI_S1P, 0x02, {0x51, 0xb7}},
        {MIPI_S1P, 0x02, {0x60, 0x22}},
        {MIPI_S1P, 0x02, {0x61, 0x00}},
        {MIPI_S1P, 0x02, {0x62, 0x19}},
        {MIPI_S1P, 0x02, {0x63, 0x10}},
        {MIPI_S1P, 0x02, {0xa0, 0x08}},
        {MIPI_S1P, 0x02, {0xa1, 0x1a}},
        {MIPI_S1P, 0x02, {0xa2, 0x27}},
        {MIPI_S1P, 0x02, {0xa3, 0x15}},
        {MIPI_S1P, 0x02, {0xa4, 0x17}},
        {MIPI_S1P, 0x02, {0xa5, 0x2a}},
        {MIPI_S1P, 0x02, {0xa6, 0x1e}},
        {MIPI_S1P, 0x02, {0xa7, 0x1f}},
        {MIPI_S1P, 0x02, {0xa8, 0x8b}},
        {MIPI_S1P, 0x02, {0xa9, 0x1b}},
        {MIPI_S1P, 0x02, {0xaa, 0x27}},
        {MIPI_S1P, 0x02, {0xab, 0x78}},
        {MIPI_S1P, 0x02, {0xac, 0x18}},
        {MIPI_S1P, 0x02, {0xad, 0x18}},
        {MIPI_S1P, 0x02, {0xae, 0x4c}},
        {MIPI_S1P, 0x02, {0xaf, 0x21}},
        {MIPI_S1P, 0x02, {0xb0, 0x27}},
        {MIPI_S1P, 0x02, {0xb1, 0x54}},
        {MIPI_S1P, 0x02, {0xb2, 0x67}},
        {MIPI_S1P, 0x02, {0xb3, 0x39}},
        {MIPI_S1P, 0x02, {0xc0, 0x08}},
        {MIPI_S1P, 0x02, {0xc1, 0x1a}},
        {MIPI_S1P, 0x02, {0xc2, 0x27}},
        {MIPI_S1P, 0x02, {0xc3, 0x15}},
        {MIPI_S1P, 0x02, {0xc4, 0x17}},
        {MIPI_S1P, 0x02, {0xc5, 0x2a}},
        {MIPI_S1P, 0x02, {0xc6, 0x1e}},
        {MIPI_S1P, 0x02, {0xc7, 0x1f}},
        {MIPI_S1P, 0x02, {0xc8, 0x8b}},
	{MIPI_S1P, 0x02, {0xc9, 0x1b}},
        {MIPI_S1P, 0x02, {0xca, 0x27}},
        {MIPI_S1P, 0x02, {0xcb, 0x78}},
        {MIPI_S1P, 0x02, {0xcc, 0x18}},
        {MIPI_S1P, 0x02, {0xcd, 0x18}},
        {MIPI_S1P, 0x02, {0xce, 0x4c}},
        {MIPI_S1P, 0x02, {0xcf, 0x21}},
        {MIPI_S1P, 0x02, {0xd0, 0x27}},
        {MIPI_S1P, 0x02, {0xd1, 0x54}},
        {MIPI_S1P, 0x02, {0xd2, 0x67}},
        {MIPI_S1P, 0x02, {0xd3, 0x39}},
        {MIPI_LCP, 0x04, {0xff, 0x98, 0x81, 0x00}},
        {MIPI_S1P, 0x02, {0x3a, 0x07}},
        {MIPI_SNP, 0x01, {0x11}},
        {MIPI_SLP, 120, {0x00}},
        {MIPI_SNP, 0x01, {0x29}},
        {MIPI_SLP, 100, {0x00}},
        {MIPI_OFF, 0x00, {0x00}},
};

void dsi_panel_write_cmd(uint8_t cmd, uint8_t data, uint8_t header)
{
        uint32_t value;

        value = (uint32_t)header | (uint32_t)cmd << 8 | (uint32_t)data << 16;
        writel(value, MIPI_DSI_CORE_BASE_ADDR + GEN_HDR);
        //usleep_range(900, 1000);
	udelay(500);
}

int dsi_panel_write_cmd_poll(uint8_t cmd, uint8_t data, uint8_t header)
{
        int ret;
        u32 val, mask;
        uint32_t value = 0;

//        ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR + CMD_PKT_STATUS,
//                        val, !(val & BIT(1)), 1000, 20000);
//	ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR + CMD_PKT_STATUS,
//                        val, !(val & BIT(1)), 20000);
//        if (ret < 0) {
//                printf("failed to get available command FIFO\n");
//                return ret;
//        }
        value = (uint32_t)header | (uint32_t)cmd << 8 | (uint32_t)data << 16;
        writel(value, MIPI_DSI_CORE_BASE_ADDR + GEN_HDR);
	udelay(500);
//        mask = BIT(0) | BIT(2);
//        ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR + CMD_PKT_STATUS,
//                        val, (val & mask) == mask, 1000, 20000);
//	ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR + CMD_PKT_STATUS,
//                        val, (val & mask) == mask, 20000);
//        if (ret < 0) {
//                printf("failed to write command FIFO\n");
//                return ret;
//        }

        return 0;
}

static int32_t dsi_panel_write_long_cmd(uint32_t len, uint8_t *tx_buf)
{
        int32_t pld_data_bytes = sizeof(uint32_t), ret;
        int32_t len_tmp = len;
        uint32_t remainder;
        uint32_t val;

        if (len < 3) {
                printf("wrong tx buf length %u for long write\n", len);
                return -EINVAL;
        }

        while (DIV_ROUND_UP(len, pld_data_bytes)) {
                if (len < pld_data_bytes) {
                        remainder = 0;
                        memcpy(&remainder, tx_buf, len);
                        writel(remainder,
				MIPI_DSI_CORE_BASE_ADDR + GEN_PLD_DATA);
                        len = 0;
                } else {
                        memcpy(&remainder, tx_buf, pld_data_bytes);
                        writel(remainder,
				MIPI_DSI_CORE_BASE_ADDR + GEN_PLD_DATA);
                        tx_buf += pld_data_bytes;
                        len -= pld_data_bytes;
                }
		udelay(500);

//                ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR +
//                				CMD_PKT_STATUS,
//                                val, !(val & BIT(3)), 1000, 20000);
//		ret = readl_poll_timeout(MIPI_DSI_CORE_BASE_ADDR + CMD_PKT_STATUS,
//                                val, !(val & BIT(3)), 20000);
//                if (ret < 0) {
//                        printf("failed to get available write FIFO\n");
//                        return ret;
//                }
        }

        dsi_panel_write_cmd_poll(len_tmp, 0x00, 0x39);//byte1 byte2 byte0

        return 0;
}

void lcd_panel_reset(void)
{
        uint32_t reg_val;

        writel(0x3, PIN_MUX_BASE_ADDR + RESET_PIN_OFFSET);

        reg_val = readl(GPIO_BASE_ADDR + GPIO_1_OUTPUT_CTRL_OFFSET);
        reg_val = (reg_val & 0xefffefff) | 0x10001000;//output high
        udelay(100000);
        reg_val = (reg_val & 0xefffefff) | 0x10000000;//output low
        udelay(120000);
        reg_val = (reg_val & 0xefffefff) | 0x10001000;//output high
        udelay(120000);
}

int mipi_dsi_panel_init(uint8_t panel_no)
{
        mipi_dsi_set_mode(1);//cmd mode
        writel(0xfffffffc, MIPI_DSI_CORE_BASE_ADDR + CMD_MODE_CFG);// 0x68
        lcd_panel_reset();

	if (panel_no == 1) {
		int i = 0;
                while (init_para_720x1280_sdb[i].cmd_type != MIPI_OFF) {
                        if (init_para_720x1280_sdb[i].cmd_type == MIPI_LCP) {
                                dsi_panel_write_long_cmd(
					init_para_720x1280_sdb[i].cmd_len,
                                        init_para_720x1280_sdb[i].cmd);
                        } else if (init_para_720x1280_sdb[i].cmd_type ==
					MIPI_S1P) {
                                dsi_panel_write_cmd_poll(
					init_para_720x1280_sdb[i].cmd[0],
					init_para_720x1280_sdb[i].cmd[1], 0x15);
                        } else if (init_para_720x1280_sdb[i].cmd_type ==
					MIPI_SNP) {
                                dsi_panel_write_cmd_poll(
					init_para_720x1280_sdb[i].cmd[0],
                                        0x00, 0x05);
                        } else if (init_para_720x1280_sdb[i].cmd_type ==
					MIPI_SLP) {
                                udelay(init_para_720x1280_sdb[i].cmd_len
						* 1000);
                        }
                        i++;
                }
                mipi_dsi_set_mode(0);//video mode
                return 0;
	} else {
		printf("%s: not support panel type,", __func__);
	}
	return 0;
}

int set_mipi_display(uint8_t panel_no)
{
        mipi_dsi_core_pre_init(panel_no);
        mipi_dsi_dpi_config(panel_no);
        if (panel_no == 0)
                mipi_dsi_video_config(&video_1080_1920);
        else if (panel_no == 1)
                mipi_dsi_video_config(&video_720_1280);
        else if (panel_no == 2)
                mipi_dsi_video_config(&video_720_1280_sdb);
        else if (panel_no == 3)
                mipi_dsi_video_config(&video_1280_720);

	udelay(1000);
        mipi_dsi_panel_init(panel_no);//0:1080*1920
	//mipi_dsi_set_mode(0);//video mode
	//mipi_dsi_vid_mode_cfg(1);//pattern mode
        return 0;
}

void iar_start(void)
{
        uint32_t value;

        value = readl(IAR_BASE_ADDR + REG_IAR_DE_REFRESH_EN);
        value = IAR_REG_SET_FILED(IAR_DPI_TV_START, 0x1, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_DE_REFRESH_EN);
        writel(0x1, IAR_BASE_ADDR + REG_IAR_UPDATE);
}

void disp_set_panel_timing(struct disp_timing *timing)
{
        uint32_t value;

        value = readl(IAR_BASE_ADDR + REG_IAR_PARAMETER_HTIM_FIELD1);
        value = IAR_REG_SET_FILED(IAR_DPI_HBP_FIELD, timing->hbp, value);
        value = IAR_REG_SET_FILED(IAR_DPI_HFP_FIELD, timing->hfp, value);
        value = IAR_REG_SET_FILED(IAR_DPI_HSW_FIELD, timing->hs, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_PARAMETER_HTIM_FIELD1);

        value = readl(IAR_BASE_ADDR + REG_IAR_PARAMETER_VTIM_FIELD1);
        value = IAR_REG_SET_FILED(IAR_DPI_VBP_FIELD, timing->vbp, value);
        value = IAR_REG_SET_FILED(IAR_DPI_VFP_FIELD, timing->vfp, value);
        value = IAR_REG_SET_FILED(IAR_DPI_VSW_FIELD, timing->vs, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_PARAMETER_VTIM_FIELD1);

        writel(timing->vfp_cnt,
                IAR_BASE_ADDR + REG_IAR_PARAMETER_VFP_CNT_FIELD12);
}

void iar_channel_base_cfg(channel_base_cfg_t *cfg)
{
        uint32_t value, channelid, pri, target_filed;
        uint32_t reg_overlay_opt_value = 0;

        channelid = cfg->channel;
	pri = cfg->pri;

        reg_overlay_opt_value = readl(IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);
        reg_overlay_opt_value =
                reg_overlay_opt_value & (0xffffffff & ~(1 << (channelid + 24)));
        reg_overlay_opt_value =
                reg_overlay_opt_value | (cfg->enable << (channelid + 24));

        writel(reg_overlay_opt_value, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);

        value = IAR_REG_SET_FILED(IAR_WINDOW_WIDTH, cfg->width, 0); //set width
        value = IAR_REG_SET_FILED(IAR_WINDOW_HEIGTH, cfg->height, value);
        writel(value, IAR_BASE_ADDR + FBUF_SIZE_ADDR_OFFSET(channelid));

        writel(cfg->buf_width,
		IAR_BASE_ADDR + FBUF_WIDTH_ADDR_OFFSET(channelid));

        value = IAR_REG_SET_FILED(IAR_WINDOW_START_X, cfg->xposition, 0);
        value = IAR_REG_SET_FILED(IAR_WINDOW_START_Y, cfg->yposition, value);
        writel(value, IAR_BASE_ADDR + WIN_POS_ADDR_OFFSET(channelid));

        target_filed = IAR_IMAGE_FORMAT_ORG_RD1 - channelid; //set format
        value = readl(IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
        value = IAR_REG_SET_FILED(target_filed, cfg->format, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);

        value = readl(IAR_BASE_ADDR + REG_IAR_ALPHA_VALUE);
        target_filed = IAR_ALPHA_RD1 - channelid; //set alpha
        value = IAR_REG_SET_FILED(target_filed, cfg->alpha, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_ALPHA_VALUE);

        writel(cfg->keycolor,
		IAR_BASE_ADDR + KEY_COLOR_ADDR_OFFSET(channelid));

        value = readl(IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);
        target_filed = IAR_LAYER_PRIORITY_1 - cfg->pri;
        value = IAR_REG_SET_FILED(target_filed, channelid, value);
	writel(value, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);

        value = IAR_REG_SET_FILED(IAR_EN_OVERLAY_PRI1, 0x1, value);
        value = IAR_REG_SET_FILED(IAR_EN_OVERLAY_PRI2, 0x1, value);
        value = IAR_REG_SET_FILED(IAR_EN_OVERLAY_PRI3, 0x1, value);
        value = IAR_REG_SET_FILED(IAR_EN_OVERLAY_PRI4, 0x1, value);

        target_filed = IAR_ALPHA_SELECT_PRI1 - pri; //set alpha sel
        value = IAR_REG_SET_FILED(target_filed, cfg->alpha_sel, value);
        target_filed = IAR_OV_MODE_PRI1 - pri; //set overlay mode
        value = IAR_REG_SET_FILED(target_filed, cfg->ov_mode, value);
        target_filed = IAR_EN_ALPHA_PRI1 - pri; //set alpha en
        value = IAR_REG_SET_FILED(target_filed, cfg->alpha_en, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);

        writel(cfg->crop_height << 16 | cfg->crop_width,
                IAR_BASE_ADDR + REG_IAR_CROPPED_WINDOW_RD1 - channelid*4);
}

void iar_output_cfg(output_cfg_t *cfg)
{
        uint32_t value;
        int ret;

	disp_set_panel_timing(&video_720x1280_touch);

	writel(0x8, IAR_BASE_ADDR + REG_IAR_DE_OUTPUT_SEL);//0x340
	writel(0x13, IAR_BASE_ADDR + REG_DISP_LCDIF_CFG);//0x800
	writel(0x3, IAR_BASE_ADDR + REG_DISP_LCDIF_PADC_RESET_N);//0x804
	writel(0x0, IAR_BASE_ADDR + REG_IAR_REFRESH_CFG);//0x204
	value = IAR_REG_SET_FILED(IAR_PANEL_WIDTH, cfg->width, 0);
	value = IAR_REG_SET_FILED(IAR_PANEL_HEIGHT, cfg->height, value);
	writel(value, IAR_BASE_ADDR + REG_IAR_PANEL_SIZE);
	value = readl(IAR_BASE_ADDR + REG_IAR_PP_CON_1);
	value = IAR_REG_SET_FILED(IAR_CONTRAST, cfg->ppcon1.contrast, value);
	value = IAR_REG_SET_FILED(IAR_THETA_SIGN, cfg->ppcon1.theta_sign, value);
	value = IAR_REG_SET_FILED(IAR_BRIGHT_EN, cfg->ppcon1.bright_en, value);
	value = IAR_REG_SET_FILED(IAR_CON_EN, cfg->ppcon1.con_en, value);
	value = IAR_REG_SET_FILED(IAR_SAT_EN, cfg->ppcon1.sat_en, value);
	value = IAR_REG_SET_FILED(IAR_HUE_EN, cfg->ppcon1.hue_en, value);
	value = IAR_REG_SET_FILED(IAR_GAMMA_ENABLE, cfg->ppcon1.gamma_en, value);
	value = IAR_REG_SET_FILED(IAR_DITHERING_EN, cfg->ppcon1.dithering_en, value);
	value = IAR_REG_SET_FILED(IAR_DITHERING_FLAG,
			cfg->ppcon1.dithering_flag, value);
	writel(value, IAR_BASE_ADDR + REG_IAR_PP_CON_1);

	value = IAR_REG_SET_FILED(IAR_OFF_BRIGHT, cfg->ppcon2.off_bright, 0);
	value = IAR_REG_SET_FILED(IAR_OFF_CONTRAST, cfg->ppcon2.off_contrast, value);
	value = IAR_REG_SET_FILED(IAR_SATURATION, cfg->ppcon2.saturation, value);
	value = IAR_REG_SET_FILED(IAR_THETA_ABS, cfg->ppcon2.theta_abs, value);
	writel(value, IAR_BASE_ADDR + REG_IAR_PP_CON_2);

	value = readl(IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
	if (cfg->big_endian == 0x1) {
		value = 0x00010000 | value;
	} else if (cfg->big_endian == 0x0) {
		value = 0xfffeffff & value;
	}
	writel(value, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
}

void iar_output_mipi(void)
{
	channel_base_cfg_t channel_base_cfg = {0};
        output_cfg_t output_cfg = {0};

	disp_set_panel_timing(&video_720x1280_touch);

	channel_base_cfg.enable = 1;
	channel_base_cfg.channel = 2;
	channel_base_cfg.pri = 0;
	channel_base_cfg.width = 720;
	channel_base_cfg.height = 1280;
	channel_base_cfg.buf_width = 720;
	channel_base_cfg.buf_height = 1280;
	channel_base_cfg.format = 4;//ARGB8888
	channel_base_cfg.alpha_sel = 0;
	channel_base_cfg.ov_mode = 0;
	channel_base_cfg.alpha_en = 1;
	channel_base_cfg.alpha = 128;
	channel_base_cfg.crop_width = 720;
	channel_base_cfg.crop_height = 1280;

	output_cfg.out_sel = 0;//mipi-dsi
	output_cfg.width = 720;
	output_cfg.height = 1280;
	output_cfg.bgcolor = 16744328;//white.

        iar_channel_base_cfg(&channel_base_cfg);
        iar_output_cfg(&output_cfg);

        writel(0x0472300f, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);
        writel(0x406, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);

	writel(FB_BASE, IAR_BASE_ADDR + REG_IAR_FBUF_ADDR_RD3);
        writel(0x1, IAR_BASE_ADDR + REG_IAR_UPDATE);

	iar_start();
}

void lcd_ctrl_init(void *lcdbase)
{
	struct mmc *mmc;
	uint64_t ret = 0;
	uint32_t reg_val;
	uint32_t iar_div_1;
	uint32_t iar_div_2;
	disk_partition_t mmc_part_info;

	mmc = find_mmc_device(0);
        if (!mmc) {
                printf("no mmc device at slot 0\n");
                return;
        }
	ret = part_get_info_by_name(mmc_get_blk_desc(mmc), "logo", &mmc_part_info);
	if (!ret) {
		printf("can't find emmc logo part, exit lcd display!!");
		return;
	}
	ret = blk_dread(mmc_get_blk_desc(mmc), mmc_part_info.start,
			mmc_part_info.size, lcdbase);
	printf("%s: blk read size is %ld\n", __func__, ret);

	reg_val = readl(CLK_BASE_REG_ADDR + VIOSYS_CLKEN);

	reg_val = readl(CLK_BASE_REG_ADDR + VIOSYS_CLK_DIV_SEL1);
	iar_div_1 = (reg_val >> 16) & 0x1f;
	iar_div_2 = (reg_val >> 21) & 0x7;

	reg_val = (reg_val & 0xff1fffff) | 0x00400000;
	writel(reg_val, CLK_BASE_REG_ADDR + VIOSYS_CLK_DIV_SEL1);
	reg_val = readl(CLK_BASE_REG_ADDR + VIOSYS_CLK_DIV_SEL1);
	iar_div_1 = (reg_val >> 16) & 0x1f;
        iar_div_2 = (reg_val >> 21) & 0x7;

	iar_output_mipi();

	set_mipi_display(1);

	return;
}

void lcd_enable(void)
{
	return;
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
	return;
}
