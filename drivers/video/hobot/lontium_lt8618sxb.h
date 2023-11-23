#ifndef LT8618SXB
#define LT8618SXB


#define _LT8618SX_ADR 0x3B // IIC Address，If CI2CA pins are low(0~400mV)
#define LT8618SXB_SLAVE_ADDR 0x3B
#define _Ver_U3_ // LT8618SXB IC version
#define _16_9_ 0x2A		// 16:9
#define _4_3_ 0x19		// 4:3
// #define _LT8618_HDCP_ //If HDMI encrypted output is not required,
// mask this macro definition

//--------------------------------------------------------//
// Use the synchronization signal inside the bt1120 signal
#define _Embedded_sync_
// need to provide external h sync, V sync and de signals to lt8618sxb
// #define _External_sync_

//--------------------------------------------------------//
// BT1120 Mapping(embedded sync): Y and C can be swapped
/******************************************************************************
  Output        16-bit        16-bit         20-bit        24-bit
  Port        Mapping1    Mapping2    Mapping        Mapping
  -------|------------|---------|---------|---------|-----------|---------|-------
  D0             Y[0]        X            X             Y[0]
  D1             Y[1]        X            X             Y[1]
  D2             Y[2]        X            Y[0]         Y[2]
  D3             Y[3]        X            Y[1]         Y[3]
  D4             Y[4]        X            Y[2]         Y[4]
  D5             Y[5]        X            Y[3]         Y[5]
  D6             Y[6]        X            Y[4]         Y[6]
  D7             Y[7]        X            Y[5]         Y[7]
  D8             C[0]        Y[0]         Y[6]         Y[8]
  D9             C[1]        Y[1]         Y[7]         Y[9]
  D10            C[2]        Y[2]         Y[8]        Y[10]
  D11         C[3]        Y[3]         Y[9]        Y[11]
  D12         C[4]        Y[4]         X            C[0]
  D13         C[5]        Y[5]         X            C[1]
  D14         C[6]        Y[6]         C[0]        C[2]
  D15         C[7]        Y[7]         C[1]        C[3]
  D16         X            C[0]        C[2]        C[4]
  D17         X            C[1]        C[3]        C[5]
  D18         X            C[2]        C[4]        C[6]
  D19         X            C[3]        C[5]        C[7]
  D20         X            C[4]        C[6]        C[8]
  D21         X            C[5]        C[7]        C[9]
  D22         X            C[6]        C[8]        C[10]
  D23         X            C[7]        C[9]        C[11]

  HS            X            X            X            X            X
  VS            X            X            X            X            X
  DE            X            X            X            X            X
 *****************************************************************************/

// YC422 Mapping(Separate  sync): // _External_sync_;
/*****************************************************************************
  Output        16-bit        16-bit         20-bit        24-bit
  Port        Mapping1    Mapping2    Mapping        Mapping
  -------|------------|---------|---------|---------|-----------|---------|-----
  D0          Y[0]        X            X             Y[0]
  D1             Y[1]        X            X             Y[1]
  D2             Y[2]        X            Y[0]         Y[2]
  D3             Y[3]        X            Y[1]         Y[3]
  D4             Y[4]        X            X             C[0]
  D5             Y[5]        X            X             C[1]
  D6             Y[6]        X            C[0]         C[2]
  D7             Y[7]        X            C[1]         C[3]
  D8             C[0]        Y[0]         Y[2]         Y[4]
  D9             C[1]        Y[1]         Y[3]         Y[5]
  D10            C[2]        Y[2]         Y[4]        Y[6]
  D11         C[3]        Y[3]         Y[5]        Y[7]
  D12         C[4]        Y[4]         Y[6]        Y[8]
  D13         C[5]        Y[5]         Y[7]        Y[9]
  D14         C[6]        Y[6]         Y[8]        Y[10]
  D15         C[7]        Y[7]         Y[9]        Y[11]
  D16         X            C[0]        C[2]        C[4]
  D17         X            C[1]        C[3]        C[5]
  D18         X            C[2]        C[4]        C[6]
  D19         X            C[3]        C[5]        C[7]
  D20         X            C[4]        C[6]        C[8]
  D21         X            C[5]        C[7]        C[9]
  D22         X            C[6]        C[8]        C[10]
  D23         X            C[7]        C[9]        C[11]

  HS            Hsync        Hsync        Hsync        Hsync        Hsync
  VS            Vsync        Vsync        Vsync        Vsync        Vsync
  DE            DE            DE            DE            DE            DE
 ******************************************************************************/

#define _16bit_ // 16bit YC
// #define _20bit_  // 20bit YC
// #define _24bit_ // 24bit YC

#ifdef _16bit_
// BT1120 16bit
// BT1120 input from D0 to D15 of LT8618SXB pins. // D0 ~ D7 Y ; D8 ~ D15 C
#define _D0_D15_In_ 0x30
// BT1120 input from D8 to D23 of LT8618SXB pins. // D8 ~ D15 Y ; D16 ~ D23 C
#define _D8_D23_In_ 0x70
// BT1120 input from D0 to D15 of LT8618SXB pins. // D0 ~ D7 C ; D8 ~ D15 Y
#define _D0_D15_In_2_ 0x00
// BT1120 input from D8 to D23 of LT8618SXB pins. // D8 ~ D15 C ; D16 ~ D23 Y
#define _D8_D23_In_2_ 0x60

#define _YC_Channel_ _D8_D23_In_2_

#define _Reg0x8246_ 0x00
// bit1/bit0 = 0:Input data color depth is 8 bit enable for BT
#define _Reg0x8248_D1_D0_ 0x00
#else
//==========================================//
#ifdef _Embedded_sync_

// 20bit
#ifdef _20bit_
// BT1120 input from D2 ~ D11 Y & D14 ~ D23 C of LT8618SXB pins.
#define _D2_D23_In_ 0x0b
// BT1120 input from D2 ~ D11 C & D14 ~ D23 Y of LT8618SXB pins.
#define _D2_D23_In_2_ 0x00

#define _YC_Channel_ 0x00
#define _Reg0x8246_ _D2_D23_In_2_ // input setting
// bit1 = 0;bit0 = 1: Input data color depth is 10 bit enable for BT
#define _Reg0x8248_D1_D0_ 0x01

#else
// 24bit
// BT1120 input from D0 ~ D11 Y & D12 ~ D23 C of LT8618SXB pins.
#define _D0_D23_In_ 0x0b
// BT1120 input from D0 ~ D11 C & D12 ~ D23 Y of LT8618SXB pins.
#define _D0_D23_In_2_ 0x00

#define _YC_Channel_ 0x00
#define _Reg0x8246_ _D0_D23_In_2_ // input setting
// bit1 = 1;bit0 = 0: Input data color depth is 12 bit enable for BT
#define _Reg0x8248_D1_D0_ 0x02

#endif
//==========================================//

#else // YC422 Mapping(Separate  sync): // _External_sync_;

#define _YC_swap_en_ 0x70
#define _YC_swap_dis_ 0x60

#define _YC_Channel_ _YC_swap_dis_ // input setting

#define _Reg0x8246_ 0X00

#ifdef _20bit_
// bit1 = 0;bit0 = 1: Input data color depth is 10 bit enable for BT
#define _Reg0x8248_D1_D0_ 0x01
#else // 24bit
// bit1 = 1;bit0 = 0: Input data color depth is 12 bit enable for BT
#define _Reg0x8248_D1_D0_ 0x02
#endif

#endif
#endif



enum
{
    _480P60_ = 0,
    _576P50_,

    _720P60_,
    _720P50_,
    _720P30_,
    _720P25_,

    _1080P60_,
    _1080P50_,
    _1080P30_,
    _1080P25_,

    _1080i60_,
    _1080i50_,

    _4K30_,

    _800x600P60_,
    _1024x768P60_,
    _1024x600_,
    _800x480_,
    _1366x768_,
    _custom_,
};
enum
{
    _32KHz = 0,
    _44d1KHz,
    _48KHz,

    _88d2KHz,
    _96KHz,
    _176Khz,
    _196KHz
};



//************************************/

enum {
	_Less_than_50M = 0x00,
	//    _Bound_25_50M,
	_Bound_50_100M,
	_Greater_than_100M
};

enum {
	H_fp = 0,
	H_sync,
	H_bp,
	H_act,
	H_tol,

	V_fp,
	V_sync,
	V_bp,
	V_act,
	V_tol,

	Vic,

	Pic_Ratio,		// Image proportion

	Clk_bound_SDR,		// SDR
	Clk_bound_DDR		// DDR
};



// int hobot_write_lt8618sxb(uint8_t reg_addr, uint8_t reg_val);
// int hobot_lt8618sxb_read_byte(struct i2c_client *client, uint32_t addr, uint8_t reg, uint8_t *val);
int  xj3_lt8618_get_edid(u8 *edid);
void LT8618SXB_Reset(void);
int xj3_hdmi_probe(struct udevice *dev);
#endif