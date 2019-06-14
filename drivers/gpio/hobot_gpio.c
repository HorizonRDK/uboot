#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <dm.h>
#include <fdtdec.h>

/* Maximum banks */
#define HOBOT_X2_GPIO_MAX_BANK	8

#define HOBOT_X2_GPIO_BANK0_NGPIO	15
#define HOBOT_X2_GPIO_BANK1_NGPIO	16
#define HOBOT_X2_GPIO_BANK2_NGPIO	15
#define HOBOT_X2_GPIO_BANK3_NGPIO	16
#define HOBOT_X2_GPIO_BANK4_NGPIO	16
#define HOBOT_X2_GPIO_BANK5_NGPIO	16
#define HOBOT_X2_GPIO_BANK6_NGPIO	16
#define HOBOT_X2_GPIO_BANK7_NGPIO	9

#define HOBOT_X2_GPIO_NR_GPIOS	(HOBOT_X2_GPIO_BANK0_NGPIO + \
            HOBOT_X2_GPIO_BANK1_NGPIO + \
            HOBOT_X2_GPIO_BANK2_NGPIO + \
            HOBOT_X2_GPIO_BANK3_NGPIO + \
            HOBOT_X2_GPIO_BANK4_NGPIO + \
            HOBOT_X2_GPIO_BANK5_NGPIO + \
            HOBOT_X2_GPIO_BANK6_NGPIO + \
            HOBOT_X2_GPIO_BANK7_NGPIO)

#define HOBOT_J2_GPIO_MAX_BANK	8
                    
#define HOBOT_J2_GPIO_BANK0_NGPIO	15
#define HOBOT_J2_GPIO_BANK1_NGPIO	16
#define HOBOT_J2_GPIO_BANK2_NGPIO	15
#define HOBOT_J2_GPIO_BANK3_NGPIO	16
#define HOBOT_J2_GPIO_BANK4_NGPIO	16
#define HOBOT_J2_GPIO_BANK5_NGPIO	16
#define HOBOT_J2_GPIO_BANK6_NGPIO	16
#define HOBOT_J2_GPIO_BANK7_NGPIO	9
                                        
#define HOBOT_J2_GPIO_NR_GPIOS	(HOBOT_J2_GPIO_BANK0_NGPIO + \
            HOBOT_J2_GPIO_BANK1_NGPIO + \
            HOBOT_J2_GPIO_BANK2_NGPIO + \
            HOBOT_J2_GPIO_BANK3_NGPIO + \
            HOBOT_J2_GPIO_BANK4_NGPIO + \
            HOBOT_J2_GPIO_BANK5_NGPIO + \
            HOBOT_J2_GPIO_BANK6_NGPIO + \
            HOBOT_J2_GPIO_BANK7_NGPIO)

#define HOBOT_GPIO_BANK0_PIN_MIN(str)	0
#define HOBOT_GPIO_BANK0_PIN_MAX(str)	(HOBOT_GPIO_BANK0_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK0_NGPIO - 1)
#define HOBOT_GPIO_BANK1_PIN_MIN(str)	(HOBOT_GPIO_BANK0_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK1_PIN_MAX(str)	(HOBOT_GPIO_BANK1_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK1_NGPIO - 1)
#define HOBOT_GPIO_BANK2_PIN_MIN(str)	(HOBOT_GPIO_BANK1_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK2_PIN_MAX(str)	(HOBOT_GPIO_BANK2_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK2_NGPIO - 1)
#define HOBOT_GPIO_BANK3_PIN_MIN(str)	(HOBOT_GPIO_BANK2_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK3_PIN_MAX(str)	(HOBOT_GPIO_BANK3_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK3_NGPIO - 1)
#define HOBOT_GPIO_BANK4_PIN_MIN(str)	(HOBOT_GPIO_BANK3_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK4_PIN_MAX(str)	(HOBOT_GPIO_BANK4_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK4_NGPIO - 1)
#define HOBOT_GPIO_BANK5_PIN_MIN(str)	(HOBOT_GPIO_BANK4_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK5_PIN_MAX(str)	(HOBOT_GPIO_BANK5_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK5_NGPIO - 1)
#define HOBOT_GPIO_BANK6_PIN_MIN(str)	(HOBOT_GPIO_BANK5_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK6_PIN_MAX(str)	(HOBOT_GPIO_BANK6_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK6_NGPIO - 1)
#define HOBOT_GPIO_BANK7_PIN_MIN(str)	(HOBOT_GPIO_BANK6_PIN_MAX(str) + 1)
#define HOBOT_GPIO_BANK7_PIN_MAX(str)	(HOBOT_GPIO_BANK7_PIN_MIN(str) + \
					HOBOT_##str##_GPIO_BANK7_NGPIO - 1)

/* Register offsets for the GPIO device */
#define X2_IO_CFG_OFFSET	0x0
#define X2_IO_PE_OFFSET	0x4
#define X2_IO_CTL_OFFSET	0x8
#define X2_IO_IN_VALUE_OFFSET	0xc

#define X2_GPIO_CFG_REG(BANK)     (X2_IO_CFG_OFFSET + (16 * BANK))
#define X2_GPIO_PE_REG(BANK)     (X2_IO_PE_OFFSET + (16 * BANK))
#define X2_GPIO_CTL_REG(BANK)     (X2_IO_CTL_OFFSET + (16 * BANK))
#define X2_GPIO_IN_VALUE_REG(BANK)     (X2_IO_IN_VALUE_OFFSET + (16 * BANK))

/* direction control bit mask */
#define X2_IO_DIR_SHIFT     16

struct hobot_gpio_platdata {
    phys_addr_t base;
    const struct hobot_platform_data *p_data;
};

/**
 * struct hobot_platform_data -  hobot gpio platform data structure
 * @label:	string to store in gpio->label
 * @ngpio:	max number of gpio pins
 * @max_bank:	maximum number of gpio banks
 * @bank_min:	this array represents bank's min pin
 * @bank_max:	this array represents bank's max pin
 */
struct hobot_platform_data {
	const char *label;
	u16 ngpio;
	u32 max_bank;
	u32 bank_min[HOBOT_X2_GPIO_MAX_BANK];
	u32 bank_max[HOBOT_X2_GPIO_MAX_BANK];
};

static const struct hobot_platform_data x2_gpio_def = {
	.label = "x2_gpio",
	.ngpio = HOBOT_X2_GPIO_NR_GPIOS,
	.max_bank = HOBOT_X2_GPIO_MAX_BANK,
	.bank_min[0] = HOBOT_GPIO_BANK0_PIN_MIN(X2),
	.bank_max[0] = HOBOT_GPIO_BANK0_PIN_MAX(X2),
	.bank_min[1] = HOBOT_GPIO_BANK1_PIN_MIN(X2),
	.bank_max[1] = HOBOT_GPIO_BANK1_PIN_MAX(X2),
	.bank_min[2] = HOBOT_GPIO_BANK2_PIN_MIN(X2),
	.bank_max[2] = HOBOT_GPIO_BANK2_PIN_MAX(X2),
	.bank_min[3] = HOBOT_GPIO_BANK3_PIN_MIN(X2),
	.bank_max[3] = HOBOT_GPIO_BANK3_PIN_MAX(X2),
	.bank_min[4] = HOBOT_GPIO_BANK4_PIN_MIN(X2),
	.bank_max[4] = HOBOT_GPIO_BANK4_PIN_MAX(X2),
	.bank_min[5] = HOBOT_GPIO_BANK5_PIN_MIN(X2),
	.bank_max[5] = HOBOT_GPIO_BANK5_PIN_MAX(X2),
    .bank_min[6] = HOBOT_GPIO_BANK6_PIN_MIN(X2),
	.bank_max[6] = HOBOT_GPIO_BANK6_PIN_MAX(X2),
	.bank_min[7] = HOBOT_GPIO_BANK7_PIN_MIN(X2),
	.bank_max[7] = HOBOT_GPIO_BANK7_PIN_MAX(X2),
};

static const struct hobot_platform_data j2_gpio_def = {
	.label = "j2_gpio",
	.ngpio = HOBOT_J2_GPIO_NR_GPIOS,
	.max_bank = HOBOT_J2_GPIO_MAX_BANK,
	.bank_min[0] = HOBOT_GPIO_BANK0_PIN_MIN(J2),
	.bank_max[0] = HOBOT_GPIO_BANK0_PIN_MAX(J2),
	.bank_min[1] = HOBOT_GPIO_BANK1_PIN_MIN(J2),
	.bank_max[1] = HOBOT_GPIO_BANK1_PIN_MAX(J2),
	.bank_min[2] = HOBOT_GPIO_BANK2_PIN_MIN(J2),
	.bank_max[2] = HOBOT_GPIO_BANK2_PIN_MAX(J2),
	.bank_min[3] = HOBOT_GPIO_BANK3_PIN_MIN(J2),
	.bank_max[3] = HOBOT_GPIO_BANK3_PIN_MAX(J2),
	.bank_min[4] = HOBOT_GPIO_BANK4_PIN_MIN(J2),
	.bank_max[4] = HOBOT_GPIO_BANK4_PIN_MAX(J2),
	.bank_min[5] = HOBOT_GPIO_BANK5_PIN_MIN(J2),
	.bank_max[5] = HOBOT_GPIO_BANK5_PIN_MAX(J2),
    .bank_min[6] = HOBOT_GPIO_BANK6_PIN_MIN(J2),
	.bank_max[6] = HOBOT_GPIO_BANK6_PIN_MAX(J2),
	.bank_min[7] = HOBOT_GPIO_BANK7_PIN_MIN(J2),
	.bank_max[7] = HOBOT_GPIO_BANK7_PIN_MAX(J2),
};

/**
 * hobot_gpio_get_bank_pin - Get the bank number and pin number within that bank
 * for a given pin in the GPIO device
 * @pin_num:	gpio pin number within the device
 * @bank_num:	an output parameter used to return the bank number of the gpio
 *		pin
 * @bank_pin_num: an output parameter used to return pin number within a bank
 *		  for the given gpio pin
 *
 * Returns the bank number and pin offset within the bank.
 */
static inline void hobot_gpio_get_bank_pin(unsigned int pin_num,
					  unsigned int *bank_num,
					  unsigned int *bank_pin_num,
					  struct udevice *dev)
{
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);
	u32 bank;

	for (bank = 0; bank < platdata->p_data->max_bank; bank++) {
		if (pin_num >= platdata->p_data->bank_min[bank] &&
		    pin_num <= platdata->p_data->bank_max[bank]) {
			*bank_num = bank;
			*bank_pin_num = pin_num -
					platdata->p_data->bank_min[bank];
			return;
		}
	}

	if (bank >= platdata->p_data->max_bank) {
		printf("Invalid bank and pin num\n");
		*bank_num = 0;
		*bank_pin_num = 0;
	}
}

static int gpio_is_valid(unsigned gpio, struct udevice *dev)
{
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	return gpio < platdata->p_data->ngpio;
}

static int check_gpio(unsigned gpio, struct udevice *dev)
{
	if (!gpio_is_valid(gpio, dev)) {
		printf("ERROR : check_gpio: invalid GPIO %d\n", gpio);
		return -1;
	}
	return 0;
}

static int hobot_gpio_direction_input(struct udevice *dev, unsigned gpio)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	if (check_gpio(gpio, dev) < 0)
		return -1;

	hobot_gpio_get_bank_pin(gpio, &bank_num, &bank_pin_num, dev);
    
	/* clear the bit in direction mode reg to set the pin as input */
	reg = readl(platdata->base + X2_GPIO_CTL_REG(bank_num));
	reg &= ~BIT(bank_pin_num + X2_IO_DIR_SHIFT);
	writel(reg, platdata->base + X2_GPIO_CTL_REG(bank_num));

	return 0;
}

static int hobot_gpio_direction_output(struct udevice *dev, unsigned gpio, int value)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	if (check_gpio(gpio, dev) < 0)
		return -1;

	hobot_gpio_get_bank_pin(gpio, &bank_num, &bank_pin_num, dev);

	/* set the GPIO pin as output */
	reg = readl(platdata->base + X2_GPIO_CTL_REG(bank_num));
	reg |= BIT(bank_pin_num + X2_IO_DIR_SHIFT);
	writel(reg, platdata->base + X2_GPIO_CTL_REG(bank_num));

	/* set the state of the pin */
	gpio_set_value(gpio, value);
    
	return 0;
}

static int hobot_gpio_get_value(struct udevice *dev, unsigned gpio)
{
	u32 data;
	unsigned int bank_num, bank_pin_num;
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	if (check_gpio(gpio, dev) < 0)
		return -1;

	hobot_gpio_get_bank_pin(gpio, &bank_num, &bank_pin_num, dev);

	data = readl(platdata->base +
			     X2_GPIO_IN_VALUE_REG(bank_num));

	return (data >> bank_pin_num) & 1;
}

static int hobot_gpio_set_value(struct udevice *dev, unsigned gpio, int value)
{
    u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	if (check_gpio(gpio, dev) < 0)
		return -1;

	hobot_gpio_get_bank_pin(gpio, &bank_num, &bank_pin_num, dev);
    reg = readl(platdata->base + X2_GPIO_CTL_REG(bank_num));

    if(value){
        reg |= BIT(bank_pin_num);
    }
    else{
        reg &= ~BIT(bank_pin_num);
    }

    writel(reg, platdata->base + X2_GPIO_CTL_REG(bank_num));

    return 0;
}

static int hobot_gpio_get_function(struct udevice *dev, unsigned offset)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	if (check_gpio(offset, dev) < 0)
		return -1;

	hobot_gpio_get_bank_pin(offset, &bank_num, &bank_pin_num, dev);

	/* set the GPIO pin as output */
	reg = readl(platdata->base + X2_GPIO_CTL_REG(bank_num));
	reg &= BIT(bank_pin_num + X2_IO_DIR_SHIFT);
	if (reg)
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static const struct dm_gpio_ops gpio_hobot_ops = {
	.direction_input	= hobot_gpio_direction_input,
	.direction_output	= hobot_gpio_direction_output,
	.get_value		= hobot_gpio_get_value,
	.set_value		= hobot_gpio_set_value,
	.get_function		= hobot_gpio_get_function,
};

static const struct udevice_id hobot_gpio_ids[] = {
	{ .compatible = "hobot,x2-gpio-1.0",
	  .data = (ulong)&x2_gpio_def},
	{ .compatible = "hobot,j2-gpio-1.0",
	  .data = (ulong)&j2_gpio_def},
	{ }
};

static int hobot_gpio_ofdata_to_platdata(struct udevice *dev)
{
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);

	platdata->base = (phys_addr_t)dev_read_addr(dev);

	platdata->p_data =
		(struct hobot_platform_data *)dev_get_driver_data(dev);

	return 0;
}

static int hobot_gpio_probe(struct udevice *dev)
{
	struct hobot_gpio_platdata *platdata = dev_get_platdata(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	const void *label_ptr;

	label_ptr = dev_read_prop(dev, "label", NULL);
	if (label_ptr) {
		uc_priv->bank_name = strdup(label_ptr);
		if (!uc_priv->bank_name)
			return -ENOMEM;
	} else {
		uc_priv->bank_name = dev->name;
	}

	if (platdata->p_data)
		uc_priv->gpio_count = platdata->p_data->ngpio;

    return 0;
}


U_BOOT_DRIVER(gpio_hobot) = {
	.name	= "gpio_hobot",
	.id	= UCLASS_GPIO,
	.ops	= &gpio_hobot_ops,
	.of_match = hobot_gpio_ids,
	.ofdata_to_platdata = hobot_gpio_ofdata_to_platdata,
	.probe	= hobot_gpio_probe,
	.platdata_auto_alloc_size = sizeof(struct hobot_gpio_platdata),
};


