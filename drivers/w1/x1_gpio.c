/***************************************************************************
* COPYRIGHT NOTICE
* Copyright 2016 Horizon Robotics, Inc.
* All rights reserved.
***************************************************************************/

#include "w1/x1_gpio.h"
#include "linux/types.h"
#include <asm/io.h>
#include "linux/delay.h"

/* GPIO PIN MUX */
#define PIN_MUX_BASE    0xA6003000
#define GPIO5_CFG (PIN_MUX_BASE + 0x50)
#define GPIO5_DIR (PIN_MUX_BASE + 0x58)
#define GPIO5_VAL (PIN_MUX_BASE + 0x5C)

extern int printf(const char *fmt, ...);

void control_gpio_5_3(void)
{
    unsigned int reg_val;
	reg_val = readl(GPIO5_CFG);
	reg_val |= 0x00003000;  //设置为gpio模式
	writel(reg_val, GPIO5_CFG);
	while (1) {
 	reg_val = readl(GPIO5_DIR);
	reg_val |= 0x00400000;
	reg_val &= 0xffffffbf; //置0
	writel(reg_val, GPIO5_DIR);
    printf("gpio5(6)set to zero \n");
	udelay(5000000);
	reg_val = readl(GPIO5_DIR);
    printf("gpio5(6)set to one\n");
	reg_val |= 0x00400040;
	writel(reg_val, GPIO5_DIR);	
	udelay(5000000);
	}
}



 //设置为gpio模式	
void set_pin_function(uint32_t gpio_index)
{
    unsigned int reg_val;
	reg_val = readl(GPIO5_CFG);
	reg_val |= 0x00003000;   //设置pin 为gpio模式；
	writel(reg_val, GPIO5_CFG);
	gpio_set_direction(gpio_index, GPIO_IN); //设置为输入;
} 


void gpio_set_direction(uint32_t gpio_index,uint32_t direction)
{
    unsigned int reg_val;
	if (direction) //输出
	{
		reg_val = readl(GPIO5_DIR);
		reg_val |= 0x00400000;
		writel(reg_val, GPIO5_DIR);		
	} else  //输入
	{
		reg_val = readl(GPIO5_DIR);
		reg_val &= 0xffbfffff;
		writel(reg_val, GPIO5_DIR);			
	}
}

void gpio_set_data(uint32_t gpio_index,uint8_t out_data)
{
    unsigned int reg_val;
	if (out_data) //置1 
	{
		reg_val = readl(GPIO5_DIR);
		reg_val |= 0x00400040;		
		writel(reg_val, GPIO5_DIR);		
	}
	else      //置0
	{
		reg_val = readl(GPIO5_DIR);
		reg_val &= 0xffffffbf;
		writel(reg_val, GPIO5_DIR);					
	}
}


int gpio_get_data( uint32_t gpio_index)
{
    uint32_t val = 0;
    val = readl(GPIO5_VAL);
    return ((val >>gpio_index) & 0x1);	
}
