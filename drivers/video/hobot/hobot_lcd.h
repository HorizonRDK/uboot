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

#ifndef DRIVERS_VIDEO_HOBOT_LCD_H_
#define DRIVERS_VIDEO_HOBOT_LCD_H_

#define MIPI_DSI_CORE_BASE_ADDR 0xA4355000
#define IAR_BASE_ADDR 0xA4301000

#define VERSION 0x0
#define PWR_UP  0x4
#define CLKMGR_CFG      0x8
#define DPI_VCID        0xc
#define DPI_COLOR_CODING        0x10
#define DPI_CFG_POL     0x14
#define DPI_LP_CMD_TIM  0x18
#define DBI_VCID        0x1c
#define DBI_CFG 0x20
#define DBI_PARTITIONING_EN     0x24
#define DBI_CMDSIZE     0x28
#define PCKHDL_CFG      0x2c
#define GEN_VCID        0x30
#define MODE_CFG        0x34
#define VID_MODE_CFG    0x38
#define VID_PKT_SIZE    0x3c
#define VID_NUM_CHUNKS  0x40
#define VID_NULL_SIZE   0x44
#define VID_HSA_TIME    0x48
#define VID_HBP_TIME    0x4c
#define VID_HLINE_TIME  0x50
#define VID_VSA_LINES   0x54
#define VID_VBP_LINES   0x58
#define VID_VFP_LINES   0x5c
#define VID_VACTIVE_LINES       0x60
#define EDPI_CMD_SIZE   0x64
#define CMD_MODE_CFG    0x68
#define GEN_HDR         0x6c
#define GEN_PLD_DATA    0x70
#define CMD_PKT_STATUS  0x74
#define TO_CNT_CFG      0x78
#define HS_RD_TO_CNT    0x7c
#define LP_RD_TO_CNT    0x80
#define HS_WR_TO_CNT    0x84
#define LP_WR_TO_CNT    0x88
#define BTA_TO_CNT      0x8c
#define SDF_3D          0x90
#define LPCLK_CTRL      0x94
#define PHY_TMR_LPCLK_CFG       0x98
#define PHY_TMR_CFG     0x9c
#define PHY_RSTZ        0xa0
#define PHY_IF_CFG      0xa4
#define PHY_ULPS_CTRL   0xa8
#define PHY_TX_TRIGGERS 0xac
#define PHY_STATUS      0xb0
#define PHY_TST_CTRL0   0xb4
#define PHY_TST_CTRL1   0xb8
#define INT_ST0         0xbc
#define INT_ST1         0xc0
#define INT_MSK0        0xc4
#define INT_MSK1        0xc8
#define PHY_CAL         0xcc
#define INT_FORCE0      0xd8
#define INT_FORCE1      0xdc
#define AUTO_ULPS_MODE  0xe0
#define AUTO_ULPS_ENTRY_DELAY   0xe4
#define AUTO_ULPS_WAKEUP_TIME   0xe8
#define DSC_PARAMETER   0xf0
#define PHY_TMR_RD_CFG  0xf4
#define AUTO_ULPS_MIN_TIME      0xf8
#define PHY_MODE        0xfc
#define VID_SHADOW_CTRL 0x100
#define DPI_VCID_ACT    0x10c
#define DPI_COLOR_CODING_ACT    0x110
#define DPI_LP_CMD_TIM_ACT      0x118
#define EDPI_TE_HW_CFG          0x11c
#define VID_MODE_CFG_ACT        0x138
#define VID_PKT_SIZE_ACT        0x13c
#define VID_NUM_CHUNKS_ACT      0x140
#define VID_NULL_SIZE_ACT       0x144
#define VID_HSA_TIME_ACT        0x148
#define VID_HBP_TIME_ACT        0x14c
#define VID_HLINE_TIME_ACT      0x150
#define VID_VSA_LINES_ACT       0x154
#define VID_VBP_LINES_ACT       0x158
#define VID_VFP_LINES_ACT       0x15c
#define VID_VACTIVE_LINES_ACT   0x160
#define VID_PKT_STATUS          0x168
#define SDF_3D_ACT              0x190
#define DSC_ENC_COREID          0x200
#define DSC_ENC_VERSION         0x204
#define DSC_ENC_FLATNESS_DET_THRES      0x208
#define DSC_ENC_DELAY           0x20c
#define DSC_ENC_COMPRESSED_LINE_SIZE    0x210
#define DSC_ENC_LINES_IN_EXCESS 0x214
#define DSC_ENC_RBUF_ADDR_LAST_LINE_ADJ 0x218
#define DSC_MODE                0x21c
#define DSC_ENC_INT_ST          0x220
#define DSC_ENC_INT_MSK         0x224
#define DSC_ENC_INT_FORCE       0x228
#define DSC_FIFO_STATUS_SELECT  0x22c
#define DSC_FIFO_STATUS         0x230
#define DSC_FIFO_STATUS2        0x234
#define DSC_ENC_PPS_0_3         0x260
#define DSC_ENC_PPS_4_7         0x264
#define DSC_ENC_PPS_8_11        0x268
#define DSC_ENC_PPS_12_15       0x26c
#define DSC_ENC_PPS_16_19       0x270
#define DSC_ENC_PPS_20_23       0x274
#define DSC_ENC_PPS_24_27       0x278
#define DSC_ENC_PPS_28_31       0x27c
#define DSC_ENC_PPS_32_35       0x280
#define DSC_ENC_PPS_36_39       0x284
#define DSC_ENC_PPS_40_43       0x288
#define DSC_ENC_PPS_44_47       0x28c
#define DSC_ENC_PPS_48_51       0x290
#define DSC_ENC_PPS_52_55       0x294
#define DSC_ENC_PPS_56_59       0x298
#define DSC_ENC_PPS_60_63       0x29c
#define DSC_ENC_PPS_64_67       0x2a0
#define DSC_ENC_PPS_68_71       0x2a4
#define DSC_ENC_PPS_72_75       0x2a8
#define DSC_ENC_PPS_76_79       0x2ac
#define DSC_ENC_PPS_80_83       0x2b0
#define DSC_ENC_PPS_84_87       0x2b4

#define REG_IAR_OVERLAY_OPT 0x0
#define REG_IAR_ALPHA_VALUE 0x4
#define REG_IAR_KEY_COLOR_RD4   0x8
#define REG_IAR_KEY_COLOR_RD3   0xC
#define REG_IAR_KEY_COLOR_RD2   0x10
#define REG_IAR_KEY_COLOR_RD1   0x14
#define REG_IAR_CROPPED_WINDOW_RD4      0x18
#define REG_IAR_CROPPED_WINDOW_RD3      0x1C
#define REG_IAR_CROPPED_WINDOW_RD2      0x20
#define REG_IAR_CROPPED_WINDOW_RD1      0x24
#define REG_IAR_IMAGE_WIDTH_FBUF_RD4    0x38
#define REG_IAR_IMAGE_WIDTH_FBUF_RD3    0x3C
#define REG_IAR_IMAGE_WIDTH_FBUF_RD2    0x40
#define REG_IAR_IMAGE_WIDTH_FBUF_RD1    0x44
#define REG_IAR_FORMAT_ORGANIZATION 0x48
#define REG_IAR_DISPLAY_POSTION_RD4 0x4C
#define REG_IAR_DISPLAY_POSTION_RD3 0x50
#define REG_IAR_DISPLAY_POSTION_RD2 0x54
#define REG_IAR_DISPLAY_POSTION_RD1 0x58
#define REG_IAR_HWC_CFG 0x5C
#define REG_IAR_HWC_SIZE        0x60
#define REG_IAR_HWC_POS 0x64
#define REG_IAR_BG_COLOR        0x68
#define REG_IAR_SRC_SIZE_UP 0x6C
#define REG_IAR_TGT_SIZE_UP 0x70
#define REG_IAR_STEP_UP 0x74
#define REG_IAR_UP_IMAGE_POSTION        0x78
#define REG_IAR_PP_CON_1        0x7C
#define REG_IAR_PP_CON_2        0x80
#define REG_IAR_THRESHOLD_RD4_3 0x84
#define REG_IAR_THRESHOLD_RD2_1 0x88
#define REG_IAR_CAPTURE_CON 0x8C
#define REG_IAR_BURST_LEN       0x94
#define REG_IAR_UPDATE  0x98
#define REG_IAR_FBUF_ADDR_RD4   0x100
#define REG_IAR_FBUF_ADDR_RD3   0x104
#define REG_IAR_FBUF_ADDR_RD2_Y 0x108
#define REG_IAR_FBUF_ADDR_RD2_U 0x10C
#define REG_IAR_FBUF_ADDR_RD2_V 0x110
#define REG_IAR_FBUF_ADDR_RD1_Y 0x114
#define REG_IAR_FBUF_ADDR_RD1_U 0x118
#define REG_IAR_FBUF_ADDR_RD1_V 0x11C
#define REG_IAR_SHADOW_FBUF_ADDR_RD4    0x120
#define REG_IAR_SHADOW_FBUF_ADDR_RD3    0x124
#define REG_IAR_SHADOW_FBUF_ADDR_RD2_Y  0x128
#define REG_IAR_SHADOW_FBUF_ADDR_RD2_U  0x12C
#define REG_IAR_SHADOW_FBUF_ADDR_RD2_V  0x130
#define REG_IAR_SHADOW_FBUF_ADDR_RD1_Y  0x134
#define REG_IAR_SHADOW_FBUF_ADDR_RD1_U  0x138
#define REG_IAR_SHADOW_FBUF_ADDR_RD1_V  0x13C
#define REG_IAR_CURRENT_CBUF_ADDR_WR_Y  0x140
#define REG_IAR_CURRENT_CBUF_ADDR_WR_UV 0x144
#define REG_IAR_PANEL_SIZE      0x200
#define REG_IAR_REFRESH_CFG 0x204
#define REG_IAR_PARAMETER_HTIM_FIELD1   0x208
#define REG_IAR_PARAMETER_VTIM_FIELD1   0x20C
#define REG_IAR_PARAMETER_VTIM_FIELD2   0x214
#define REG_IAR_PARAMETER_VFP_CNT_FIELD12       0x218
#define REG_IAR_GAMMA_X1_X4_R   0x21C
#define REG_IAR_GAMMA_X5_X8_R   0x220
#define REG_IAR_GAMMA_X9_X12_R  0x224
#define REG_IAR_GAMMA_X13_X15_R 0x228
#define REG_IAR_GAMMA_X1_X4_G   0x22C
#define REG_IAR_GAMMA_X5_X8_G   0x230
#define REG_IAR_GAMMA_X9_X12_G  0x234
#define REG_IAR_GAMMA_X13_X15_G 0x238
#define REG_IAR_GAMMA_X1_X4_B   0x23C
#define REG_IAR_GAMMA_X5_X8_B   0x240
#define REG_IAR_GAMMA_X9_X12_B  0x244
#define REG_IAR_GAMMA_X13_X15_B 0x248
#define REG_IAR_GAMMA_Y1_Y3_R   0x24C
#define REG_IAR_GAMMA_Y4_Y7_R   0x250
#define REG_IAR_GAMMA_Y8_Y11_R  0x254
#define REG_IAR_GAMMA_Y12_Y15_R 0x258
#define REG_IAR_GAMMA_Y1_Y3_G   0x25C
#define REG_IAR_GAMMA_Y4_Y7_G   0x260
#define REG_IAR_GAMMA_Y8_Y11_G  0x264
#define REG_IAR_GAMMA_Y12_Y15_G 0x268
#define REG_IAR_GAMMA_Y1_Y3_B   0x26C
#define REG_IAR_GAMMA_Y4_Y7_B   0x270
#define REG_IAR_GAMMA_Y8_Y11_B  0x274
#define REG_IAR_GAMMA_Y12_Y15_B 0x278
#define REG_IAR_GAMMA_Y16_RGB   0x27C
#define REG_IAR_AUTO_DBI_REFRESH_CNT    0x280
#define REG_IAR_HWC_SRAM_WR 0x300
#define REG_IAR_PALETTE 0x304
#define REG_IAR_DE_SRCPNDREG    0x308
#define REG_IAR_DE_INTMASK      0x30C
#define REG_IAR_DE_SETMASK      0x310
#define REG_IAR_DE_UNMASK       0x314
#define REG_IAR_DE_REFRESH_EN   0x318
#define REG_IAR_DE_CONTROL_WO   0x31C
#define REG_IAR_DE_STATUS       0x320
#define REG_IAR_DE_REVISION 0x330
#define REG_IAR_DE_MAXOSNUM_RD  0x334
#define REG_IAR_DE_MAXOSNUM_WR  0x338
#define REG_IAR_DE_SW_RST       0x33C
#define REG_IAR_DE_OUTPUT_SEL   0x340
#define REG_IAR_DE_AR_CLASS 0x400
#define REG_IAR_DE_AR_CLASS_WEIGHT      0x404

#define REG_DISP_LCDIF_CFG 0x800
#define REG_DISP_LCDIF_PADC_RESET_N 0x804

enum mipi_dsi_reg_cfg_e {
        SHUTDOWNZ_FILED,//0
        TX_ESC_CLK_DIVISION_FILED,//1
        TO_CLK_DIVISION_FILED,//2
        PHY_TX_REQUEST_CLK_HS_FILED,//3
        AUTO_CLKLANE_CTRL_FILED,//4
        N_LANES_FILED,//5
        PHY_STOP_WAIT_TIME_FILED,//6
        PHY_SHUTDOWNZ_FILED,//7
        PHY_RSTZ_FILED,//8
        PHY_ENABLE_CLK_FILED,//9
        PHY_FORCE_PLL_FILED,//10
        DPI_VCID_FILED,//11
        DPI_COLOR_CODING_FILED,//12
        VID_PKT_SIZE_FILED,//13
        VID_NUM_CHUNKS_FILED,//14
        VID_NULL_SIZE_FILED,//15
        VID_HSA_TIME_FILED,//16
        VID_HBP_TIME_FILED,//17
        VID_HLINE_TIME_FILED,//18
        VSA_LINES_FILED,//19
        VBP_LINES_FILED,//20
        VFP_LINES_FILED,//21
        V_ACTIVE_LINES_FILED,//22
        CMD_VIDEO_MODE_FILED,//23
        VID_MODE_TYPE_FILED,//24
        LP_VSA_EN_FILED,//25
        LP_VBP_EN_FILED,//26
        LP_VFP_EN_FILED,//27
        LP_VACT_EN_FILED,//28
        LP_HBP_EN_FILED,//29
        LP_HFP_EN_FILED,//30
        FRAME_BTA_ACK_EN_FILED,//31
        LP_CMD_EN_FILED,//32
        VPG_EN_FILED,//33
        VPG_MODE_FILED,//34
	VPG_ORIENTATION_FILED,//35
        TEAR_FX_EN_FILED,//36
        ACK_RQST_EN_FILED,//37
        GEN_SW_0P_TX_FILED,//38
        GEN_SW_1P_TX_FILED,//39
        GEN_SW_2P_TX_FILED,//40
        GEN_SR_0P_TX_FILED,//41
        GEN_SR_1P_TX_FILED,//42
        GEN_SR_2P_TX_FILED,//43
        GEN_LW_TX_FILED,//44
        DCS_SW_0P_TX_FILED,//45
        DCS_SW_1P_TX_FILED,//46
        DCS_SR_0P_TX_FILED,//47
        DCS_LW_TX_FILED,//48
        MAX_RD_PKT_SIZE_FILED,//49
        GEN_DT_FILED,//50
        GEN_VC_FILED,//51
        GEN_WC_LS_BYTE_FILED,//52
        GEN_WC_MS_BYTE_FILED,//53
};


enum _mipi_dsi_table_e {
        TABLE_MASK = 0,
        TABLE_OFFSET,
        TABLE_MAX,
};

enum iar_Reg_cfg_e {
        IAR_ALPHA_SELECT_PRI4,
        IAR_ALPHA_SELECT_PRI3,
        IAR_ALPHA_SELECT_PRI2,
        IAR_ALPHA_SELECT_PRI1,
        IAR_EN_RD_CHANNEL4,
        IAR_EN_RD_CHANNEL3,
        IAR_EN_RD_CHANNEL2,
        IAR_EN_RD_CHANNEL1,
        IAR_LAYER_PRIORITY_4,
        IAR_LAYER_PRIORITY_3,
        IAR_LAYER_PRIORITY_2,
        IAR_LAYER_PRIORITY_1,
        IAR_EN_OVERLAY_PRI4,
        IAR_EN_OVERLAY_PRI3,
        IAR_EN_OVERLAY_PRI2,
        IAR_EN_OVERLAY_PRI1,
        IAR_OV_MODE_PRI4,
        IAR_OV_MODE_PRI3,
        IAR_OV_MODE_PRI2,
        IAR_OV_MODE_PRI1,
        IAR_EN_ALPHA_PRI4,
        IAR_EN_ALPHA_PRI3,
        IAR_EN_ALPHA_PRI2,
        IAR_EN_ALPHA_PRI1,

        IAR_ALPHA_RD4,
        IAR_ALPHA_RD3,
        IAR_ALPHA_RD2,
        IAR_ALPHA_RD1,

        IAR_WINDOW_HEIGTH,
        IAR_WINDOW_WIDTH,

        IAR_WINDOW_START_Y,
        IAR_WINDOW_START_X,

        IAR_BT601_709_SEL,
        IAR_RGB565_CONVERT_SEL,
        IAR_IMAGE_FORMAT_ORG_RD4,
        IAR_IMAGE_FORMAT_ORG_RD3,
        IAR_IMAGE_FORMAT_ORG_RD2,
        IAR_IMAGE_FORMAT_ORG_RD1,

        IAR_LAYER_TOP_Y,
        IAR_LAYER_LEFT_X,

        IAR_HWC_COLOR,
        IAR_HWC_COLOR_EN,
        IAR_HWC_EN,

        IAR_HWC_HEIGHT,
        IAR_HWC_WIDTH,

        IAR_HWC_TOP_Y,
        IAR_HWC_LEFT_X,

        IAR_BG_COLOR,

        IAR_SRC_HEIGTH,
        IAR_SRC_WIDTH,

        IAR_TGT_HEIGTH,
        IAR_TGT_WIDTH,

        IAR_STEP_Y,
        IAR_STEP_X,

        IAR_UP_IMAGE_TOP_Y,
        IAR_UP_IMAGE_LEFT_X,

        IAR_CONTRAST,
	IAR_THETA_SIGN,
        IAR_UP_SCALING_EN,
        IAR_ALGORITHM_SELECT,
        IAR_BRIGHT_EN,
        IAR_CON_EN,
        IAR_SAT_EN,
        IAR_HUE_EN,
        IAR_GAMMA_ENABLE,
        IAR_DITHERING_EN,
        IAR_DITHERING_FLAG,

        IAR_OFF_BRIGHT,
        IAR_OFF_CONTRAST,
        IAR_SATURATION,
        IAR_THETA_ABS,

        IAR_THRESHOLD_RD4,
        IAR_THRESHOLD_RD3,

        IAR_THRESHOLD_RD2,
        IAR_THRESHOLD_RD1,

        IAR_CAPTURE_INTERLACE,
        IAR_CAPTURE_MODE,
        IAR_SOURCE_SEL,
        IAR_OUTPUT_FORMAT,

        IAR_SLICE_LINES,

        IAR_BURST_LEN_WR,
        IAR_BURST_LEN_RD,

        IAR_UPDATE,

        IAR_PANEL_HEIGHT,
        IAR_PANEL_WIDTH,

        IAR_ITU_R_656_EN,
        IAR_UV_SEQUENCE,
        IAR_P3_P2_P1_P0,
        IAR_YCBCR_OUTPUT,
        IAR_PIXEL_RATE,
        IAR_ODD_POLARITY,
        IAR_DEN_POLARITY,
        IAR_VSYNC_POLARITY,
        IAR_HSYNC_POLARITY,
        IAR_INTERLACE_SEL,
        IAR_PANEL_COLOR_TYPE,
        IAR_DBI_REFRESH_MODE,

        IAR_DPI_HBP_FIELD,
        IAR_DPI_HFP_FIELD,
        IAR_DPI_HSW_FIELD,

        IAR_DPI_VBP_FIELD,
        IAR_DPI_VFP_FIELD,
        IAR_DPI_VSW_FIELD,

        IAR_DPI_HBP_FIELD2,
        IAR_DPI_HFP_FIELD2,
        IAR_DPI_HSW_FIELD2,

        IAR_DPI_VBP_FIELD2,
        IAR_DPI_VFP_FIELD2,
        IAR_DPI_VSW_FIELD2,

        IAR_PARAMETER_VFP_CNT,

        IAR_GAMMA_XY_D_R,
        IAR_GAMMA_XY_C_R,
        IAR_GAMMA_XY_B_R,
        IAR_GAMMA_XY_A_R,

        IAR_GAMMA_Y16_R,
        IAR_GAMMA_Y16_G,
        IAR_GAMMA_Y16_B,

        IAR_HWC_SRAM_ADDR,
        IAR_HWC_SRAM_D,

        IAR_PALETTE_INDEX,
        IAR_PALETTE_DATA,

        IAR_INT_WR_FIFO_FULL,
        IAR_INT_CBUF_SLICE_START,
        IAR_INT_CBUF_SLICE_END,
        IAR_INT_CBUF_FRAME_END,
        IAR_INT_FBUF_FRAME_END,
        IAR_INT_FBUF_SWITCH_RD4,
        IAR_INT_FBUF_SWITCH_RD3,
        IAR_INT_FBUF_SWITCH_RD2,
        IAR_INT_FBUF_SWITCH_RD1,
        IAR_INT_FBUF_START_RD4,
        IAR_INT_FBUF_START_RD3,
        IAR_INT_FBUF_START_RD2,
        IAR_INT_FBUF_START_RD1,
        IAR_INT_FBUF_START,
        IAR_INT_BUFFER_EMPTY_RD4,
        IAR_INT_BUFFER_EMPTY_RD3,
        IAR_INT_BUFFER_EMPTY_RD2,
        IAR_INT_BUFFER_EMPTY_RD1,
        IAR_INT_THRESHOLD_RD4,
        IAR_INT_THRESHOLD_RD3,
        IAR_INT_THRESHOLD_RD2,
        IAR_INT_THRESHOLD_RD1,
        IAR_INT_FBUF_END_RD4,
        IAR_INT_FBUF_END_RD3,
        IAR_INT_FBUF_END_RD2,
	IAR_INT_FBUF_END_RD1,
        IAR_INT_VSYNC,

        IAR_AUTO_DBI_REFRESH_EN,
        IAR_DPI_TV_START,

        IAR_FIELD_ODD_CLEAR,
        IAR_DBI_START,
        IAR_CAPTURE_EN,

        IAR_DMA_STATE_RD4,
        IAR_DMA_STATE_RD3,
        IAR_DMA_STATE_RD2,
        IAR_DMA_STATE_RD1,
        IAR_DMA_STATE_WR,
        IAR_DPI_STATE,
        IAR_DBI_STATE,
        IAR_CAPTURE_STATUS,
        IAR_REFRESH_STATUS,

        IAR_CUR_BUF,
        IAR_CUR_BUFGRP,
        IAR_STALL_OCCUR,
        IAR_ERROR_OCCUR,

        IAR_MAXOSNUM_DMA0_RD1,
        IAR_MAXOSNUM_DMA1_RD1,
        IAR_MAXOSNUM_DMA2_RD1,
        IAR_MAXOSNUM_DMA0_RD2,
        IAR_MAXOSNUM_DMA1_RD2,
        IAR_MAXOSNUM_DMA2_RD2,
        IAR_MAXOSNUM_RD3,
        IAR_MAXOSNUM_RD4,

        IAR_MAXOSNUM_DMA0_WR,
        IAR_MAXOSNUM_DMA1_WR,

        IAR_WR_RST,
        IAR_RD_RST,

        IAR_IAR_OUTPUT_EN,
        IAR_RGB_OUTPUT_EN,
        IAR_BT1120_OUTPUT_EN,
        IAR_MIPI_OUTPUT_EN,

        IAR_AR_CLASS3_WEIGHT,
        IAR_AR_CLASS2_WEIGHT,
};

typedef struct _channel_base_cfg_t {
        uint32_t channel;
        uint32_t enable;
        uint32_t pri;
        uint32_t width;
        uint32_t height;
        uint32_t buf_width;
        uint32_t buf_height;
        uint32_t xposition;
        uint32_t yposition;
        uint32_t format;
        //buf_addr_t  bufaddr;
        uint32_t alpha;
        uint32_t keycolor;
        uint32_t alpha_sel;
        uint32_t ov_mode;
        uint32_t alpha_en;
        uint32_t crop_width;
        uint32_t crop_height;
} channel_base_cfg_t;

typedef struct _ppcon1_cfg_t {
        uint32_t dithering_flag;
        uint32_t dithering_en;
        uint32_t gamma_en;
        uint32_t hue_en;
        uint32_t sat_en;
        uint32_t con_en;
        uint32_t bright_en;
        uint32_t theta_sign;
        uint32_t contrast;
} ppcon1_cfg_t;

typedef struct _ppcon2_cfg_t {
        uint32_t theta_abs; //ppcon2
        uint32_t saturation;
        uint32_t off_contrast;
        uint32_t off_bright;
        float gamma_value;
} ppcon2_cfg_t;

typedef struct _refresh_cfg_t {
        uint32_t dbi_refresh_mode;  //refresh mode
        uint32_t panel_corlor_type;
        uint32_t interlace_sel;
        uint32_t odd_polarity;
        uint32_t pixel_rate;
        uint32_t ycbcr_out;
        uint32_t uv_sequence;
        uint32_t itu_r656_en;

        uint32_t auto_dbi_refresh_cnt;
        uint32_t auto_dbi_refresh_en;
} refresh_cfg_t;

typedef struct _output_cfg_t {
        uint32_t bgcolor;
        uint32_t out_sel;
        uint32_t width;
        uint32_t height;
        uint32_t big_endian;
        uint32_t display_addr_type;
        uint32_t display_cam_no;
        uint32_t display_addr_type_layer1;
        uint32_t display_cam_no_layer1;
        ppcon1_cfg_t ppcon1;
        ppcon2_cfg_t ppcon2;
        refresh_cfg_t refresh_cfg;
        uint32_t panel_type;
        uint32_t rotate;
        uint32_t user_control_disp;
        uint32_t user_control_disp_layer1;
} output_cfg_t;

struct disp_timing {
        uint32_t hbp;
        uint32_t hfp;
        uint32_t hs;
        uint32_t vbp;
        uint32_t vfp;
        uint32_t vs;
        uint32_t vfp_cnt;
};

struct video_timing {
        uint32_t vid_pkt_size;
        uint32_t vid_num_chunks;
        uint32_t vid_null_size;
        uint32_t vid_hsa;
        uint32_t vid_hbp;
        uint32_t vid_hline_time;
        uint32_t vid_vsa;
        uint32_t vid_vbp;
        uint32_t vid_vfp;
        uint32_t vid_vactive_line;
};

#endif // DRIVERS_VIDEO_HOBOT_LCD_H_
