#ifndef _PANGU_GPIO_H_
#define _PANGU_GPIO_H_
//#include <stdint.h>
#include <linux/types.h>
/*GPIO PIN value defination*/
#define GPIO_PIN_GPIO0           (0)
#define GPIO_PIN_GPIO1           (1)
#define GPIO_PIN_GPIO2           (2)
#define GPIO_PIN_GPIO3           (3)
#define GPIO_PIN_GPIO4           (4)
#define GPIO_PIN_GPIO5           (5)
#define GPIO_PIN_GPIO6           (6)
#define GPIO_PIN_GPIO7           (7)
#define GPIO_PIN_GPIO8           (8)
#define GPIO_PIN_GPIO9           (9)
#define GPIO_PIN_GPIO10          (10)
#define GPIO_PIN_GPIO11          (11)
#define GPIO_PIN_GPIO12          (12)
#define GPIO_PIN_GPIO13          (13)
#define GPIO_PIN_GPIO14          (14)

#define GPIO_PIN_GPIO30          (30)  /* this gpio for secure ic in IPC */

/*GPIO PIN direct defination*/
#define GPIO_DIR_IN              (0)
#define GPIO_DIR_OUT             (1)
void gpio_get_value( uint32_t bit, uint32_t *value );
void gpio_set_value( uint32_t bit, uint32_t dir, uint32_t value );

#endif /*_PANGU_GPIO_H_*/
