// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017-2018 STMicroelectronics - All Rights Reserved
 * Author(s): Philippe Cornu <philippe.cornu@st.com> for STMicroelectronics.
 *	      Yannick Fertre <yannick.fertre@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <video.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <dm/device-internal.h>
#include "../videomodes.h"

#include "hobot_iar.h"
#include "lontium_lt8618sxb.h"

DECLARE_GLOBAL_DATA_PTR;

#define FB_BASE 0x2E000000
#define FB_BASE_SIZE 0x7e9000 // 1920*1080*4
#define XJ3_PIN_MUX_BASE_ADDR (0xA6004000)
#define XJ3_BT1120_REG_ADDR_START (0x138)
#define XJ3_BT1120_REG_ADDR_END (0x178)

const unsigned int g_mipi_dsi_reg_cfg_table[][3] = {
    /*reg mask      reg offset*/
    {0x1, 0x0},    // 0
    {0xff, 0x0},   // 1
    {0xff, 0x8},   // 2
    {0x1, 0x0},    // 3
    {0x1, 0x1},    // 4
    {0x3, 0x0},    // 5
    {0xff, 0x8},   // 6
    {0x1, 0x0},    // 7
    {0x1, 0x1},    // 8
    {0x1, 0x2},    // 9
    {0x1, 0x3},    // 10
    {0x3, 0x0},    // 11
    {0xf, 0x0},    // 12
    {0x3fff, 0x0}, // 13
    {0x1fff, 0x0}, // 14
    {0x1fff, 0x0}, // 15
    {0xfff, 0x0},  // 16
    {0xfff, 0x0},  // 17
    {0x7fff, 0x0}, // 18
    {0x3ff, 0x0},  // 19
    {0x3ff, 0x0},  // 20
    {0x3ff, 0x0},  // 21
    {0x3fff, 0x0}, // 22
    {0x1, 0x0},    // 23
    {0x3, 0x0},    // 24
    {0x1, 0x8},    // 25
    {0x1, 0x9},    // 26
    {0x1, 0xa},    // 27
    {0x1, 0xb},    // 28
    {0x1, 0xc},    // 29
    {0x1, 0xd},    // 30
    {0x1, 0xe},    // 31
    {0x1, 0xf},    // 32
    {0x1, 0x10},   // 33
    {0x1, 0x14},   // 34
    {0x1, 0x18},   // 35
    {0x1, 0x0},    // 36
    {0x1, 0x1},    // 37
    {0x1, 0x8},    // 38
    {0x1, 0x9},    // 39
    {0x1, 0xa},    // 40
    {0x1, 0xb},    // 41
    {0x1, 0xc},    // 42
    {0x1, 0xd},    // 43
    {0x1, 0xe},    // 44
    {0x1, 0x10},   // 45
    {0x1, 0x11},   // 46
    {0x1, 0x12},   // 47
    {0x1, 0x13},   // 48
    {0x1, 0x18},   // 49
    {0x3f, 0x0},   // 50
    {0x3, 0x6},    // 51
    {0xff, 0x8},   // 52
    {0xff, 0x10},  // 53
};

const unsigned int g_iarReg_cfg_table[][3] = {
    /*reg mask      reg offset*/
    {0x1, 0x1f}, /*ALPHA_SELECT_PRI4*/
    {0x1, 0x1e}, /*ALPHA_SELECT_PRI3*/
    {0x1, 0x1d}, /*ALPHA_SELECT_PRI2*/
    {0x1, 0x1c}, /*ALPHA_SELECT_PRI1*/
    {0x1, 0x1b}, /*EN_RD_CHANNEL4*/
    {0x1, 0x1a}, /*EN_RD_CHANNEL3*/
    {0x1, 0x19}, /*EN_RD_CHANNEL2*/
    {0x1, 0x18}, /*EN_RD_CHANNEL1*/
    {0x3, 0x16}, /*LAYER_PRIORITY_4*/
    {0x3, 0x14}, /*LAYER_PRIORITY_3*/
    {0x3, 0x12}, /*LAYER_PRIORITY_2*/
    {0x3, 0x10}, /*LAYER_PRIORITY_1*/
    {0x1, 0xf},  /*EN_OVERLAY_PRI4*/
    {0x1, 0xe},  /*EN_OVERLAY_PRI3*/
    {0x1, 0xd},  /*EN_OVERLAY_PRI2*/
    {0x1, 0xc},  /*EN_OVERLAY_PRI1*/
    {0x3, 0xa},  /*OV_MODE_PRI4*/
    {0x3, 0x8},  /*OV_MODE_PRI3*/
    {0x3, 0x6},  /*OV_MODE_PRI2*/
    {0x3, 0x4},  /*OV_MODE_PRI1*/
    {0x1, 0x3},  /*EN_ALPHA_PRI4*/
    {0x1, 0x2},  /*EN_ALPHA_PRI3*/
    {0x1, 0x1},  /*EN_ALPHA_PRI2*/
    {0x1, 0x0},  /*EN_ALPHA_PRI1*/
    /*X2_IAR_ALPHA_VALUE*/
    {0xff, 0x18}, /*ALPHA_RD4*/
    {0xff, 0x10}, /*ALPHA_RD3*/
    {0xff, 0x8},  /*ALPHA_RD2*/
    {0xff, 0x0},  /*ALPHA_RD1*/
    /*X2_IAR_CROPPED_WINDOW_RD_x*/
    {0x7ff, 0x10}, /*WINDOW_HEIGTH*/
    {0x7ff, 0x0},  /*WINDOW_WIDTH*/
    /*X2_IAR_WINDOW_POSITION_FBUF_RD_x*/
    {0xfff, 0x10}, /*WINDOW_START_Y*/
    {0xfff, 0x0},  /*WINDOW_START_X*/
                   /*X2_IAR_FORMAT_ORGANIZATION*/
    {0x1, 0xf},    /*BT601_709_SEL*/
    {0x1, 0xe},    /*RGB565_CONVERT_SEL*/
    {0x7, 0xb},    /*IMAGE_FORMAT_ORG_RD4*/
    {0x7, 0x8},    /*IMAGE_FORMAT_ORG_RD3*/
    {0xf, 0x4},    /*IMAGE_FORMAT_ORG_RD2*/
    {0xf, 0x0},    /*IMAGE_FORMAT_ORG_RD1*/
    /*X2_IAR_DISPLAY_POSTION_RD_x*/
    {0x7ff, 0x10}, /*LAYER_TOP_Y*/
    {0x7ff, 0x0},  /*LAYER_LEFT_X*/
    /*X2_IAR_HWC_CFG*/
    {0xffffff, 0x2}, /*HWC_COLOR*/
    {0x1, 0x1},      /*HWC_COLOR_EN*/
    {0x1, 0x0},      /*HWC_EN*/
    /*X2_IAR_HWC_SIZE*/
    {0x7ff, 0x10}, /*HWC_HEIGHT*/
    {0x7ff, 0x0},  /*HWC_WIDTH*/
    /*X2_IAR_HWC_POS*/
    {0x7ff, 0x10}, /*HWC_TOP_Y*/
    {0x7ff, 0x0},  /*HWC_LEFT_X*/
    /*X2_IAR_BG_COLOR*/
    {0xffffff, 0x0}, /*BG_COLOR*/
    /*X2_IAR_SRC_SIZE_UP*/
    {0x7ff, 0x10}, /*SRC_HEIGTH*/
    {0x7ff, 0x0},  /*SRC_WIDTH*/
    /*X2_IAR_TGT_SIZE_UP*/
    {0x7ff, 0x10}, /*TGT_HEIGTH*/
    {0x7ff, 0x0},  /*TGT_WIDTH*/
    /*X2_IAR_STEP_UP*/
    {0xfff, 0x10}, /*STEP_Y*/
    {0xfff, 0x0},  /*STEP_X*/
    /*X2_IAR_UP_IMAGE_POSTION*/
    {0x7ff, 0x10}, /*UP_IMAGE_TOP_Y*/
    {0x7ff, 0x0},  /*UP_IMAGE_LEFT_X*/
    /*X2_IAR_PP_CON_1*/
    {0x3f, 0xa}, /*CONTRAST*/
    {0x1, 0x9},  /*THETA_SIGN*/
    {0x1, 0x8},  /*UP_SCALING_EN*/
    {0x1, 0x7},  /*ALGORITHM_SELECT*/
    {0x1, 0x6},  /*BRIGHT_EN*/
    {0x1, 0x5},  /*CON_EN*/
    {0x1, 0x4},  /*SAT_EN*/
    {0x1, 0x3},  /*HUE_EN*/
    {0x1, 0x2},  /*GAMMA_ENABLE*/
    {0x1, 0x1},  /*DITHERING_EN*/
    {0x1, 0x0},  /*DITHERING_FLAG*/
    /*X2_IAR_PP_CON_2*/
    {0xff, 0x18}, /*OFF_BRIGHT*/
    {0xff, 0x10}, /*OFF_CONTRAST*/
    {0xff, 0x8},  /*SATURATION*/
    {0xff, 0x0},  /*THETA_ABS*/
    /*X2_IAR_THRESHOLD_RD4_3*/
    {0x7ff, 0x10}, /*THRESHOLD_RD4*/
    {0x7ff, 0x0},  /*THRESHOLD_RD3*/
    /*X2_IAR_THRESHOLD_RD2_1*/
    {0x7ff, 0x10}, /*THRESHOLD_RD2*/
    {0x7ff, 0x0},  /*THRESHOLD_RD1*/
    /*X2_IAR_CAPTURE_CON*/
    {0x1, 0x6}, /*CAPTURE_INTERLACE*/
    {0x1, 0x5}, /*CAPTURE_MODE*/
    {0x3, 0x3}, /*SOURCE_SEL*/
    {0x7, 0x0}, /*OUTPUT_FORMAT*/
    /*X2_IAR_CAPTURE_SLICE_LINES*/
    {0xfff, 0x0}, /*SLICE_LINES*/
    /*X2_IAR_BURST_LEN*/
    {0xf, 0x4}, /*BURST_LEN_WR*/
    {0xf, 0x0}, /*BURST_LEN_RD*/
    /*X2_IAR_UPDATE*/
    {0x1, 0x0}, /*UPDATE*/
    /*X2_IAR_PANEL_SIZE*/
    {0x7ff, 0x10}, /*PANEL_HEIGHT*/
    {0x7ff, 0x0},  /*PANEL_WIDTH*/
                   /*X2_IAR_REFRESH_CFG*/
    {0x1, 0x11},   /*ITU_R_656_EN*/
    {0x1, 0x10},   /*UV_SEQUENCE*/
    {0xf, 0xc},    /*P3_P2_P1_P0*/
    {0x1, 0xb},    /*YCBCR_OUTPUT*/
    {0x3, 0x9},    /*PIXEL_RATE*/
    {0x1, 0x8},    /*ODD_POLARITY*/
    {0x1, 0x7},    /*DEN_POLARITY*/
    {0x1, 0x6},    /*VSYNC_POLARITY*/
    {0x1, 0x5},    /*HSYNC_POLARITY*/
    {0x1, 0x4},    /*INTERLACE_SEL*/
    {0x3, 0x2},    /*PANEL_COLOR_TYPE*/
    {0x1, 0x1},    /*DBI_REFRESH_MODE*/
    /*X2_IAR_PARAMETER_HTIM_FIELD1*/
    {0x3ff, 0x14}, /*DPI_HBP_FIELD*/
    {0x3ff, 0xa},  /*DPI_HFP_FIELD*/
    {0x3ff, 0x0},  /*DPI_HSW_FIELD*/
    /*X2_IAR_PARAMETER_VTIM_FIELD1*/
    {0x3ff, 0x14}, /*DPI_VBP_FIELD*/
    {0x3ff, 0xa},  /*DPI_VFP_FIELD*/
    {0x3ff, 0x0},  /*DPI_VSW_FIELD*/
    /*X2_IAR_PARAMETER_HTIM_FIELD2*/
    {0x3ff, 0x14}, /*DPI_HBP_FIELD2*/
    {0x3ff, 0xa},  /*DPI_HFP_FIELD2*/
    {0x3ff, 0x0},  /*DPI_HSW_FIELD2*/
    /*X2_IAR_PARAMETER_VTIM_FIELD2*/
    {0x3ff, 0x14}, /*DPI_VBP_FIELD2*/
    {0x3ff, 0xa},  /*DPI_VFP_FIELD2*/
    {0x3ff, 0x0},  /*DPI_VSW_FIELD2*/
    /*X2_IAR_PARAMETER_VFP_CNT_FIELD12*/
    {0xffff, 0x0}, /*PARAMETER_VFP_CNT*/
    /*X2_IAR_GAMMA_X1_X4_R*/
    {0xff, 0x18}, /*GAMMA_XY_D_R*/
    {0xff, 0x10}, /*GAMMA_XY_C_R*/
    {0xff, 0x8},  /*GAMMA_XY_B_R*/
    {0xff, 0x0},  /*GAMMA_XY_A_R*/
                  /*X2_IAR_GAMMA_Y16_RGB*/
    {0xff, 0x10}, /*GAMMA_Y16_R*/
    {0xff, 0x8},  /*GAMMA_Y16_G*/
    {0xff, 0x0},  /*GAMMA_Y16_B*/
    /*X2_IAR_HWC_SRAM_WR*/
    {0x1ff, 0x10}, /*HWC_SRAM_ADDR*/
    {0xffff, 0x0}, /*HWC_SRAM_D*/
    /*X2_IAR_PALETTE*/
    {0xff, 0x18},    /*PALETTE_INDEX*/
    {0xffffff, 0x0}, /*PALETTE_DATA*/
    /*X2_IAR_DE_SRCPNDREG*/
    {0x1, 0x1a}, /*INT_WR_FIFO_FULL*/
    {0x1, 0x19}, /*INT_CBUF_SLICE_START*/
    {0x1, 0x18}, /*INT_CBUF_SLICE_END*/
    {0x1, 0x17}, /*INT_CBUF_FRAME_END*/
    {0x1, 0x16}, /*INT_FBUF_FRAME_END*/
    {0x1, 0x15}, /*INT_FBUF_SWITCH_RD4*/
    {0x1, 0x14}, /*INT_FBUF_SWITCH_RD3*/
    {0x1, 0x13}, /*INT_FBUF_SWITCH_RD2*/
    {0x1, 0x12}, /*INT_FBUF_SWITCH_RD1*/
    {0x1, 0x11}, /*INT_FBUF_START_RD4*/
    {0x1, 0x10}, /*INT_FBUF_START_RD3*/
    {0x1, 0xf},  /*INT_FBUF_START_RD2*/
    {0x1, 0xe},  /*INT_FBUF_START_RD1*/
    {0x1, 0xd},  /*INT_FBUF_START*/
    {0x1, 0xc},  /*INT_BUFFER_EMPTY_RD4*/
    {0x1, 0xb},  /*INT_BUFFER_EMPTY_RD3*/
    {0x1, 0xa},  /*INT_BUFFER_EMPTY_RD2*/
    {0x1, 0x9},  /*INT_BUFFER_EMPTY_RD1*/
    {0x1, 0x8},  /*INT_THRESHOLD_RD4*/
    {0x1, 0x7},  /*INT_THRESHOLD_RD3*/
    {0x1, 0x6},  /*INT_THRESHOLD_RD2*/
    {0x1, 0x5},  /*INT_THRESHOLD_RD1*/
    {0x1, 0x4},  /*INT_FBUF_END_RD4*/
    {0x1, 0x3},  /*INT_FBUF_END_RD3*/
    {0x1, 0x2},  /*INT_FBUF_END_RD2*/
    {0x1, 0x1},  /*INT_FBUF_END_RD1*/
    {0x1, 0x0},  /*INT_VSYNC*/
    /*X2_IAR_DE_REFRESH_EN*/
    {0x1, 0x1}, /*AUTO_DBI_REFRESH_EN*/
    {0x1, 0x0}, /*DPI_TV_START*/
    /*X2_IAR_DE_CONTROL_WO*/
    {0x1, 0x2}, /*FIELD_ODD_CLEAR*/
    {0x1, 0x1}, /*DBI_START*/
    {0x1, 0x0}, /*CAPTURE_EN*/
    /*X2_IAR_DE_STATUS*/
    {0xf, 0x1c}, /*DMA_STATE_RD4*/
    {0xf, 0x18}, /*DMA_STATE_RD3*/
    {0xf, 0x14}, /*DMA_STATE_RD2*/
    {0xf, 0x10}, /*DMA_STATE_RD1*/
    {0xf, 0xc},  /*DMA_STATE_WR*/
    {0x7, 0x9},  /*DPI_STATE*/
    {0xf, 0x5},  /*DBI_STATE*/
    {0x1, 0x1},  /*CAPTURE_STATUS*/
    {0x1, 0x0},  /*REFRESH_STATUS*/
    /*X2_IAR_AXI_DEBUG_STATUS1*/
    {0xffff, 0x3}, /*CUR_BUF*/
    {0x1, 0x2},    /*CUR_BUFGRP*/
    {0x1, 0x1},    /*STALL_OCCUR*/
    {0x1, 0x0},    /*ERROR_OCCUR*/
    /*X2_IAR_DE_MAXOSNUM_RD*/
    {0x7, 0x1c}, /*MAXOSNUM_DMA0_RD1*/
    {0x7, 0x18}, /*MAXOSNUM_DMA1_RD1*/
    {0x7, 0x14}, /*MAXOSNUM_DMA2_RD1*/
    {0x7, 0x10}, /*MAXOSNUM_DMA0_RD2*/
    {0x7, 0xc},  /*MAXOSNUM_DMA1_RD2*/
    {0x7, 0x8},  /*MAXOSNUM_DMA2_RD2*/
    {0x7, 0x4},  /*MAXOSNUM_RD3*/
    {0x7, 0x0},  /*MAXOSNUM_RD4*/
    /*X2_IAR_DE_MAXOSNUM_WR*/
    {0x7, 0x4}, /*MAXOSNUM_DMA0_WR*/
    {0x7, 0x0}, /*MAXOSNUM_DMA1_WR*/
                /*X2_IAR_DE_SW_RST*/
    {0x1, 0x1}, /*WR_RST*/
    {0x1, 0x0}, /*RD_RST*/
    /*X2_IAR_DE_OUTPUT_SEL*/
    {0x1, 0x3}, /*IAR_OUTPUT_EN*/
    {0x1, 0x2}, /*RGB_OUTPUT_EN*/
    {0x1, 0x1}, /*BT1120_OUTPUT_EN*/
    {0x1, 0x0}, /*MIPI_OUTPUT_EN*/
    /*X2_IAR_DE_AR_CLASS_WEIGHT*/
    {0xffff, 0x10}, /*AR_CLASS3_WEIGHT*/
    {0xffff, 0x0},  /*AR_CLASS2_WEIGHT*/
};

static void xj3_bt1120_pinmux_switch(void)
{
    u_int32_t bt1120_range = XJ3_PIN_MUX_BASE_ADDR + XJ3_BT1120_REG_ADDR_END;
    u_int32_t bt1120_addr = XJ3_PIN_MUX_BASE_ADDR + XJ3_BT1120_REG_ADDR_START + 4;
    writel(0x000002A0, XJ3_PIN_MUX_BASE_ADDR + XJ3_BT1120_REG_ADDR_START);
    for (; bt1120_addr <= bt1120_range; bt1120_addr += 4)
    {
        uint32_t reg_value = readl(bt1120_addr);

        // set_zero
        reg_value &= ~0x03;

        // write back
        writel(reg_value, bt1120_addr);
        printf("BT1120 [reg]: %x,[val]: %x\n", bt1120_addr, reg_value);
    }
}

static void iar_set_timing(struct disp_timing *timing)
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

static void iar_channel_base_cfg(channel_base_cfg_t *cfg)
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

    value = IAR_REG_SET_FILED(IAR_WINDOW_WIDTH, cfg->width, 0); // set width
    value = IAR_REG_SET_FILED(IAR_WINDOW_HEIGTH, cfg->height, value);
    writel(value, IAR_BASE_ADDR + FBUF_SIZE_ADDR_OFFSET(channelid));

    writel(cfg->buf_width,
           IAR_BASE_ADDR + FBUF_WIDTH_ADDR_OFFSET(channelid));

    value = IAR_REG_SET_FILED(IAR_WINDOW_START_X, cfg->xposition, 0);
    value = IAR_REG_SET_FILED(IAR_WINDOW_START_Y, cfg->yposition, value);
    writel(value, IAR_BASE_ADDR + WIN_POS_ADDR_OFFSET(channelid));

    target_filed = IAR_IMAGE_FORMAT_ORG_RD1 - channelid; // set format
    value = readl(IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
    value = IAR_REG_SET_FILED(target_filed, cfg->format, value);
    writel(value, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);

    value = readl(IAR_BASE_ADDR + REG_IAR_ALPHA_VALUE);
    target_filed = IAR_ALPHA_RD1 - channelid; // set alpha
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

    target_filed = IAR_ALPHA_SELECT_PRI1 - pri; // set alpha sel
    value = IAR_REG_SET_FILED(target_filed, cfg->alpha_sel, value);
    target_filed = IAR_OV_MODE_PRI1 - pri; // set overlay mode
    value = IAR_REG_SET_FILED(target_filed, cfg->ov_mode, value);
    target_filed = IAR_EN_ALPHA_PRI1 - pri; // set alpha en
    value = IAR_REG_SET_FILED(target_filed, cfg->alpha_en, value);
    writel(value, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);

    writel(cfg->crop_height << 16 | cfg->crop_width,
           IAR_BASE_ADDR + REG_IAR_CROPPED_WINDOW_RD1 - channelid * 4);
}

static void iar_output_cfg(output_cfg_t *cfg, int mode)
{
    uint32_t value;
    int ret;

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
    if (cfg->big_endian == 0x1)
    {
        value = 0x00010000 | value;
    }
    else if (cfg->big_endian == 0x0)
    {
        value = 0xfffeffff & value;
    }
    writel(value, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);

    if (mode == 1) // HDMI
    {
        writel(0xa, IAR_BASE_ADDR + REG_IAR_DE_OUTPUT_SEL);
        // color config
        value = readl(IAR_BASE_ADDR + REG_IAR_REFRESH_CFG);
        value = IAR_REG_SET_FILED(IAR_PANEL_COLOR_TYPE, 2, value);
        // yuv444
        value = IAR_REG_SET_FILED(IAR_YCBCR_OUTPUT, 1, value);
        // convert ycbcr
        writel(value, IAR_BASE_ADDR + REG_IAR_REFRESH_CFG);
        value = readl(IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
        value = IAR_REG_SET_FILED(IAR_BT601_709_SEL, 1, value);
        writel(value, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);
    }
    else if (mode == 0)
    {
        // TODO: Add MIPI-DSI
    }
}

void iar_start(void)
{
    uint32_t value;

    value = readl(IAR_BASE_ADDR + REG_IAR_DE_REFRESH_EN);
    value = IAR_REG_SET_FILED(IAR_DPI_TV_START, 0x1, value);
    writel(value, IAR_BASE_ADDR + REG_IAR_DE_REFRESH_EN);
    writel(0x1, IAR_BASE_ADDR + REG_IAR_UPDATE);
}

static void xj3_iar_enable(struct xj3_iar_priv *priv)
{
    iar_start();
}

static int xj3_iar_set_mode(struct xj3_iar_priv *priv)
{
    struct edid1_info edid1 = {0};
    struct edid_detailed_timing *dtd = (struct edid_detailed_timing *)edid1.monitor_details.timing;
    struct ctfb_res_modes mode;
    struct disp_timing *iar_timing = &priv->display_timing;
    channel_base_cfg_t *channel_base_cfg = &priv->iar_channel_base_cfg;
    output_cfg_t *output_cfg = &priv->iar_output_cfg;
    int ret = 0;
    int i = 0;
    if (priv->output_mode == 1) // HDMI
    {
        xj3_bt1120_pinmux_switch();
        ret = xj3_lt8618_get_edid((u8 *)&edid1);
        if (ret)
        {
            printf("LT8618: Get EDID data failed!\n");
        }

        ret = edid_check_info(&edid1);
        if (ret)
        {
            printf("EDID: invalid EDID data\n");
            // ret = -EINVAL;
        }
        if (edid1.version != 1 || (edid1.revision < 3 && !EDID1_INFO_FEATURE_PREFERRED_TIMING_MODE(edid1)))
        {
            printf("EDID: unsupported version %d.%d\n",
                   edid1.version, edid1.revision);
            // return -EINVAL;
        }
        if (!ret)
        {
            for (i = 0; i < 4; i++, dtd++)
            {
                ret = video_edid_dtd_to_ctfb_res_modes(dtd, &mode);
                if (ret == 0)
                    break;
            }
        }
        else
        {
            //If EDID is not found or invalid, use the default 1080@60Hz output
            
            printf("No valid screens found,using default mode: 1080P@60Hz\n");
            extern const struct ctfb_res_modes res_mode_init[RES_MODES_COUNT];
            mode = res_mode_init[RES_MODE_1920x1080];
        }
        if (i == 4)
        {
            printf("EDID: no usable detailed timing found\n");
            return -ENOENT;
        }
#ifdef CONFIG_XJ3_DISPLAY_DEBUG
        edid_print_info(&edid1);
#endif
        // TIMING SETTING START...
        memset(iar_timing, 0, sizeof(struct disp_timing));
        iar_timing->hfp = mode.right_margin;
        iar_timing->hbp = mode.left_margin;
        iar_timing->vfp = mode.lower_margin;
        iar_timing->vbp = mode.upper_margin;
        iar_timing->hs = mode.hsync_len;
        iar_timing->vs = mode.vsync_len;
        iar_timing->vfp_cnt = 0X0A;
        iar_set_timing(iar_timing);
        xj3_display_debug("hfp:%d,hbp:%d,vfp:%d,vbp:%d,hs:%d,vs:%d\n", iar_timing->hfp, iar_timing->hbp, iar_timing->vfp,
                          iar_timing->vbp, iar_timing->hs, iar_timing->vs);
        priv->iar_bt1120_pix_clk = mode.pixclock_khz * 1000;
        xj3_display_debug("iar_bt1120_pix_clk:%d\n", priv->iar_bt1120_pix_clk);
        // TIMING SETTING END...

        // CHANNEL && OUTPUT SETTING START...
        memset(channel_base_cfg, 0, sizeof(channel_base_cfg_t));
        channel_base_cfg->enable = 1;
        channel_base_cfg->channel = 2;
        channel_base_cfg->pri = 0;
        channel_base_cfg->width = mode.xres;
        channel_base_cfg->height = mode.yres;
        channel_base_cfg->buf_width = mode.xres;
        channel_base_cfg->buf_height = mode.yres;
        channel_base_cfg->format = 2; // see xj3 iar datasheet
        channel_base_cfg->alpha_sel = 0;
        channel_base_cfg->ov_mode = 0;
        channel_base_cfg->alpha_en = 1;
        channel_base_cfg->alpha = 128;
        channel_base_cfg->crop_width = mode.xres;
        channel_base_cfg->crop_height = mode.yres;

        memset(output_cfg, 0, sizeof(output_cfg_t));
        output_cfg->out_sel = priv->output_mode; // HDMI:1
        output_cfg->width = mode.xres;
        output_cfg->height = mode.yres;
        output_cfg->bgcolor = 16744328; // white.

        iar_channel_base_cfg(channel_base_cfg);
        iar_output_cfg(output_cfg, priv->output_mode);
        // CHANNEL && OUTPUT SETTING END...
    }
    else if (priv->output_mode == 0)
    {
        // TODO: Add MIPI-DSI
    }
    writel(0x0472300f, IAR_BASE_ADDR + REG_IAR_OVERLAY_OPT);
    // FIXME: REG_IAR_FORMAT_ORGANIZATION register's value  is fixed on 0x206
    // Which is unpack rgb888
    writel(0x206, IAR_BASE_ADDR + REG_IAR_FORMAT_ORGANIZATION);

    writel(FB_BASE, IAR_BASE_ADDR + REG_IAR_FBUF_ADDR_RD3);
    writel(0x1, IAR_BASE_ADDR + REG_IAR_UPDATE);
    return 0;
}

static int xj3_iar_probe(struct udevice *dev)
{
    struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);
    struct xj3_iar_priv *priv = dev_get_priv(dev);
    struct video_priv *uc_priv = dev_get_uclass_priv(dev);
    const void *blob = gd->fdt_blob;
    const int node = dev_of_offset(dev);
    char *iar_output_mode = NULL;
    fdt_addr_t base;
    fdt_size_t size;

    base = FB_BASE;
    size = FB_BASE_SIZE;

    plat->base = base;
    plat->size = size;

    priv->output_mode = 1; // 0:MIPI-DSI 1:HDMI
    int rate, ret;

    xj3_display_debug("%s start1!\n", __func__);
    ret = clk_get_by_index(dev, 0, &priv->iar_pix_clk);
    if (ret)
    {
        printf("%s: peripheral clock get error %d\n", __func__, ret);
        return ret;
    }
    xj3_display_debug("%s start2!\n", __func__);

    ret = clk_enable(&priv->iar_pix_clk);
    if (ret)
    {
        printf("%s: peripheral clock enable error %d\n",
               __func__, ret);
        return ret;
    }

    iar_output_mode = env_get("iar_output_mode");

    if (iar_output_mode == NULL)
    {
        printf("%s: IAR output mode not define,using HDMI output as default\n",
              __func__);
        priv->output_mode = 1;
    }

    if (iar_output_mode != NULL && strstr(iar_output_mode, "HDMI"))
        priv->output_mode = 1;
    else if (iar_output_mode != NULL && strstr(iar_output_mode, "MIPI-DSI"))
        priv->output_mode = 0;

    ret = xj3_iar_set_mode(priv);
    if (ret)
    {
        printf("set mode fail!\n");
    }

    rate = clk_set_rate(&priv->iar_pix_clk, priv->iar_bt1120_pix_clk);
    if (rate < 0)
    {
        printf("%s: fail to set pixel clock %d hz %d hz\n",
               __func__, priv->iar_bt1120_pix_clk, rate);
        return rate;
    }

    xj3_iar_enable(priv);

    // gd->fb_base = FB_BASE;

    if (priv->output_mode == 1)
    {
        xj3_hdmi_probe(dev);

        uc_priv->xsize = priv->iar_channel_base_cfg.width;
        uc_priv->ysize = priv->iar_channel_base_cfg.height;
        uc_priv->bpix = VIDEO_BPP32;
        uc_priv->rot = 0;

        xj3_display_debug("%s hdmi probe done!\n", __func__);
    }
    else
    {
        // TODO:Add MIPI-DSI
    }
    video_set_flush_dcache(dev, true);
    xj3_display_debug("%s video_set_flush_dcache done!\n", __func__);
    return 0;
}

static int xj3_iar_bind(struct udevice *dev)
{
    struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);
    uc_plat->size = FB_BASE_SIZE;
    debug("%s: frame buffer max size %d bytes\n", __func__, uc_plat->size);

    return 0;
}

static const struct udevice_id xj3_iar_ids[] = {
    {.compatible = "hobot,hobot-iar"},
    {}};

U_BOOT_DRIVER(xj3_iar) = {
    .name = "iar_display",
    .id = UCLASS_VIDEO,
    .of_match = xj3_iar_ids,
    //.bind = xj3_iar_bind,
    .probe = xj3_iar_probe,
    .priv_auto_alloc_size = sizeof(struct xj3_iar_priv),
    .flags = DM_FLAG_PRE_RELOC,
};
