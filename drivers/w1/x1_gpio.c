/***************************************************************************
* COPYRIGHT NOTICE
* Copyright 2016 Horizon Robotics, Inc.
* All rights reserved.
***************************************************************************/
/**
 * @file     pangu_gpio.c
 * @brief    pangu gpio driver
 * @author   tarryzhang (tianyu.zhang@hobot.cc)
 * @date     2017/8/2
 * @version  V1.0
 * @par      Horizon Robotics
 */
#include "w1/x1_gpio.h"
#include "linux/types.h"
//#include "sys/tiny_printf.h"

#define SYSC_PIN_SHARE1_L_REG           0
#define SYSC_PIN_SHARE1_M_REG           0
#define SYSC_PIN_SHARE1_H_REG           0
#define SYSC_PIN_SHARE2_REG             0
#define GPIO_BASE               0xB0000000    // 0xB0000000~0xB0000FFF

#define GPIO_REG_BASE       GPIO_BASE
/* direction of data flow register
** 0 input 1 output
*/
#define GPIO_REG_DIR        (GPIO_REG_BASE+0x00)
/* control mode select register
** 0 for software mode control
** 1 for hardware mode control
*/
#define GPIO_REG_CTRL       (GPIO_REG_BASE+0x04)
/* output data set register */
#define GPIO_REG_SET        (GPIO_REG_BASE+0x08)
/* output data clear register */
#define GPIO_REG_CLR        (GPIO_REG_BASE+0x0C)
/* output data register */
#define GPIO_REG_ODATA      (GPIO_REG_BASE+0x10)
/* input data register */
#define GPIO_REG_IDATA      (GPIO_REG_BASE+0x14)
/* interrupt enable reigster*/
#define GPIO_REG_IEN        (GPIO_REG_BASE+0x18)
/* interrupt sensor register
** 0 edge-detecting
** 1 level-detecting
*/
#define GPIO_REG_IS         (GPIO_REG_BASE+0x1C)
/* interrupt both edge register
 * 0 is single edge-sensitive either falling edge or rising edge
 * 1 is both edges
 */
#define GPIO_REG_IBE        (GPIO_REG_BASE+0x20)
/* interrupt event register
** 0 interrupt is triggered in falling edge
** 1 triggered in rising edge
**/
#define GPIO_REG_IEV        (GPIO_REG_BASE+0x24)
/* raw interrupt status register
** 1 interrupt pending
** 0 interrupt not pending
*/
#define GPIO_REG_RIS        (GPIO_REG_BASE+0x28)
/* interrupt mask register */
#define GPIO_REG_IM         (GPIO_REG_BASE+0x2C)
/* masked interrupt status register */
#define GPIO_REG_MIS        (GPIO_REG_BASE+0x30)
/* interrupt clear register */
#define GPIO_REG_IC         (GPIO_REG_BASE+0x34)
/* delete bounce flag */
#define GPIO_REG_DB         (GPIO_REG_BASE+0x38)
/* define filtered glitch */
#define GPIO_REG_DFG        (GPIO_REG_BASE+0x3C)

#define reg_r(a)              (*(volatile uint32_t *)(a))
#define reg_w(a,v)            (*(volatile uint32_t *)(a) = (v))

#define SET_ATTRIBUTE(raw_val,attr_offset,attr_length,atrr_val) \
		(((uint32_t)raw_val&\
          (~((((uint32_t)1L<<(uint32_t)attr_length)-(uint32_t)1L)<<(uint32_t)attr_offset)))|\
         ((uint32_t)atrr_val<<(uint32_t)attr_offset))

void sysc_pmux_gpio( uint32_t gpio_index )
{
    uint32_t gpio_csr_offset, tmp32;
    if ( gpio_index < 5 ) { //GPIO[4:0]
        gpio_csr_offset = gpio_index + 2;
        tmp32 = reg_r(SYSC_PIN_SHARE1_L_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_L_REG, tmp32);
    } else if ( gpio_index < 11 ) { //GPIO[10:5]
        gpio_csr_offset = gpio_index + 3;
        tmp32 = reg_r(SYSC_PIN_SHARE1_L_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_L_REG, tmp32);
    } else if ( gpio_index < 15 ) { //GPIO[14:11]
        gpio_csr_offset = gpio_index + 7;
        tmp32 = reg_r(SYSC_PIN_SHARE1_L_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_L_REG, tmp32);
    } else if ( gpio_index < 21 ) { //GPIO[20:15]
        gpio_csr_offset = gpio_index + 11;
        tmp32 = reg_r(SYSC_PIN_SHARE1_L_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_L_REG, tmp32);
    } else if ( gpio_index < 30 ) { //GPIO[20:15]
        gpio_csr_offset = gpio_index - 21;
        tmp32 = reg_r(SYSC_PIN_SHARE1_M_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_M_REG, tmp32);
    } else if ( gpio_index < 32 ) { //GPIO[31:30]
        gpio_csr_offset = (gpio_index - 30);
        tmp32 = reg_r(SYSC_PIN_SHARE1_L_REG);
        tmp32 = SET_ATTRIBUTE(tmp32, gpio_csr_offset, 1, 1);
        reg_w(SYSC_PIN_SHARE1_L_REG, tmp32);
    } else {
        printf("gpio index invalid\n");
    }
    //uart_printf("gpio_index[%d] = 0x%x\r\n", gpio_index, tmp32);
}

void gpio_reg_config(uint32_t gpio_index, uint32_t reg_address, uint32_t reg_val)
{
	uint32_t raw_cfg;
	raw_cfg=reg_r(reg_address);
	raw_cfg = SET_ATTRIBUTE(raw_cfg,gpio_index,1,(reg_val & 0x01));
	reg_w(reg_address,raw_cfg);
}

void gpio_set_direction(uint32_t gpio_index,uint32_t direction)
{
	gpio_reg_config(gpio_index,GPIO_REG_DIR,direction);
}

void gpio_out_data(uint32_t gpio_index,uint8_t out_data)
{
	gpio_reg_config(gpio_index,GPIO_REG_ODATA,out_data);
}


int gpio_get_input_value( uint32_t bit)
{
    uint32_t val = 0;
    val = reg_r(GPIO_REG_IDATA);
    return ((val >>bit) & 0x1);
}
