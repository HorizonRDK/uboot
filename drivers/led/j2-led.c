#include <common.h>
#include <dm.h>
#include <dm/lists.h>
#include <j2-led.h>
#include <asm/gpio.h>

struct j2_led_dev gj2_led;

static int j2_led_get_devtree_data(const void *blob, struct j2_led_dev *j2_led)
{
    int led_node;
    int ret;
    
    led_node = fdt_path_offset(blob, "/j2-leds");
    if(led_node < 0)
        return -1;

    ret = fdtdec_get_int(blob, led_node, "red-gpio", -1);
    if(ret < 0)
        return -1;
    j2_led->red_gpio = ret;

    ret = fdtdec_get_int(blob, led_node, "green-gpio", -1);
    if(ret < 0)
        return -1;
    j2_led->green_gpio = ret;

    return 0;
}

static int j2_led_switch(unsigned int led, int on)
{
    int ret;
      
    ret = gpio_request(led, "j2_led_gpio");
    if (ret && ret != -EBUSY) {
        printf("%s: requesting pin %u failed\n", __func__, led);
        return -1;
    }

    ret = gpio_direction_output(led, on);
    gpio_free(led);
    
    return ret;
}

int j2_led_init(void)
{
    int ret;
    const void *blob = gd->fdt_blob;
    struct j2_led_dev *j2_led = &gj2_led;

    ret = j2_led_get_devtree_data(blob, j2_led);
    if(ret){
        printf("j2_led: get device tree data error\n");
        return ret;
    }
    
    ret = j2_led_switch(j2_led->green_gpio, 1);
    if(ret){
        printf("j2_led: j2 led operate error\n");
        return ret;
    }

    return 0;
}

