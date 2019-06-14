#ifndef __J2_LED_H__
#define __J2_LED_H__

struct j2_led_dev{
    unsigned int red_gpio;
    unsigned int green_gpio;
};

int j2_led_init(void);

#endif

