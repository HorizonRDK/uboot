#include <common.h>
#include <command.h>
#include <errno.h>
#include <dm.h>
#include <asm/gpio.h>

#define ETH_MAC_CFG0_GPIO   73      //GPIO4[11]
#define ETH_MAC_CFG1_GPIO   74      //GPIO4[12]

enum j2id {
	J2A,
	J2C,
	J2B,
	J2D,
};

char * j2id_name[] = {
    "j2a",
    "j2c",
    "j2b",
    "j2d",
};

static inline int j2id_gpio_get_value(unsigned int gpio)
{
    int value;

    value = gpio_request(gpio, "eth_mac_cfg0_gpio");
	if (value && value != -EBUSY) {
		printf("j2id: requesting pin %u failed\n", gpio);
		return -1;
	}
    gpio_direction_input(gpio);
    value = gpio_get_value(gpio);
    gpio_free(gpio);

    return value;
}

static int do_get_j2id(enum j2id *id){
    int ret;
    int value;

    ret = j2id_gpio_get_value(ETH_MAC_CFG0_GPIO);
    if(ret < 0)
        return -1;
    value = ret;
    
    ret = j2id_gpio_get_value(ETH_MAC_CFG1_GPIO);
    if(ret < 0)
        return -1;

    value |= (ret << 1); 

    switch(value){
        case J2A: *id = J2A; break;
        case J2B: *id = J2B; break;
        case J2C: *id = J2C; break;
        case J2D: *id = J2D; break;
        default: *id = -1; return -1;
    }

    return 0;
}

static int do_j2id(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    enum j2id id;
    int ret;
    
	if (argc > 1)
show_usage:
		return CMD_RET_USAGE;
    ret = do_get_j2id(&id);
    if(ret)
        goto show_usage;
    printf("ID: %s\n", j2id_name[id]);
    
    return 0;
}

U_BOOT_CMD(j2id, 1, 0, do_j2id,
	   "get ID of current j2 on matrix v2.0 quad",
	   "without any argument\n");


