#ifndef _PANGU_GPIO_H_
#define _PANGU_GPIO_H_
//#include <stdint.h>
#include <linux/types.h>


#define GPIO_PIN_GPIO30          (30)  /* this gpio for secure ic in IPC */

/*GPIO PIN direct defination*/
#define GPIO_IN              (0)
#define GPIO_OUT             (1)
 //设置为gpio模式	
void set_pin_function(uint32_t gpio_index);
void gpio_set_direction(uint32_t gpio_index,uint32_t direction);
void gpio_set_data(uint32_t gpio_index,uint8_t out_data);
int gpio_get_data( uint32_t gpio_index);


#endif /*_PANGU_GPIO_H_*/
