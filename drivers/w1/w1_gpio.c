/*
 * w1-gpio - GPIO w1 bus master driver
 *
 * Copyright (C) 2007 Ville Syrjala <syrjala@sci.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "stdio.h"
#include "w1/w1_family.h"
#include "w1/w1_int.h"
#include "w1/w1_gpio.h"
#include "linux/delay.h"
#include "linux/string.h"
#include "w1/x1_gpio.h"
#include "w1/w1_ds28e1x.h"

int w1_max_slave_count = 1;            /* suppose only one slave device */
int w1_max_slave_ttl = 10;

extern struct w1_master                *master_total;

/* 25 28~33 ok !*/
#define TICK_TO_US(t)           ((0xffffffff-(t))*30/1000)

/* 20 35 40 failed !!*/
#define PROT_BIT_AUTHWRITE_WRITE	0x10
#define PROT_BIT_WRITE_PROTECTION	0x40
#define PROT_BIT_RESET			0x00

struct w1_slave sl_satic;

W1_LIST_HEAD(w1_masters);

static W1_LIST_HEAD(w1_families);

/**
 * w1_register_family() - register a device family driver
 * @newf:	family to register
 */

static int w1_search_count = 5;           /* set w1_process loop max cnt */

static int w1_enable_pullup = 1;

struct w1_master              pdev;

struct w1_bus_master          master;
struct w1_gpio_platform_data  pdata;


/* for the time delay may fix by wilbur */
void udelay_mod(unsigned long usec)
{
	udelay(usec);
}



static struct w1_master *w1_alloc_dev(uint32_t id, int slave_count, struct w1_bus_master *bus_dev)

{
    struct w1_master *dev;
    dev =&pdev;
    dev->bus_master       = bus_dev;
	dev->max_slave_count  = slave_count;
	dev->slave_count      = 0;
	dev->initialized      = 0;
	dev->match_slave      = 0;
	dev->id               = id;
	dev->search_count     = w1_search_count;
	dev->enable_pullup    = w1_enable_pullup;

	/* 1 for w1_process to decrement
	 * 1 for __w1_remove_master_device to decrement
	 */
	W1_INIT_LIST_HEAD(&dev->slist);
	return dev;
}


/**
 * w1_add_master_device() - registers a new master device
 * @master:	master bus device to register
 */
struct w1_master *w1_add_master_device(struct w1_bus_master *master)
{
	struct w1_master *dev;
	int id;

	/* validate minimum functionality */
	if (!(master->touch_bit && master->reset_bus) &&
	    !(master->write_bit && master->read_bit) &&
	    !(master->write_byte && master->read_byte && master->reset_bus)) {
		return NULL;
	}
	/* Search for the first available id (starting at 1). */
	id = 0;
        #if 0
		struct *entry;
		int found;
        do {
        ++id;
        found = 0;
        list_for_each_entry(entry, &w1_masters, w1_master_entry) {
            if (entry->id == id) {
                found = 1;
                break;
            }
		}

	} while (found);

	#endif

        dev = w1_alloc_dev(id, w1_max_slave_count,master);

        if (!dev) {
            return NULL;
        }

        memcpy(dev->bus_master, master, sizeof(struct w1_bus_master));
        dev->initialized = 1;

        list_add_tail(&dev->w1_master_entry, &w1_masters);
        //printf("w1_add_master_device  ok !!!\n");
        return dev;
}

struct w1_slave *w1_slave_match_one(struct w1_master *dev,
        u8 fid)
{
        list_h_t *ent, *n;
        struct w1_slave *sl;
        list_for_each_safe(ent, n, &dev->slist) {
            sl = (struct w1_slave *)ent;
                //if (sl->reg_num.family == fid ) {
                if (sl->family->fid == fid ) {
                    return sl;
                }
        }
        return NULL;
}

struct w1_slave *w1_slave_search_device(struct w1_master *dev,
	struct w1_reg_num *rn)
{
        list_h_t *ent, *n;
        struct w1_slave *sl;
        list_for_each_safe(ent, n, &dev->slist) {
            sl = (struct w1_slave *)ent;
                if (sl->reg_num.family == rn->family &&
                                sl->reg_num.id == rn->id &&
                                sl->reg_num.crc == rn->crc) {
                    return sl;
                }
				//printf("search family %x,\n",rn->family);

        }
        return NULL;
}


static int w1_family_notify(unsigned long action, struct w1_slave *sl)
{
    struct w1_family_ops *fops;
    int err;

	fops = sl->family->fops;

	if (!fops)
            return 0;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
            /* if the family driver needs to initialize something... */
            if (fops->add_slave) {
                        err = fops->add_slave(sl);
                        if (err < 0) {
                            return err;
                        }
            }

        break;
        default:
            return -1;
        }
        return 0;
}

static int __w1_attach_slave_device(struct w1_slave *sl)
{
	int err;
	err =w1_family_notify(BUS_NOTIFY_ADD_DEVICE, sl);
        if (err < 0) {
            return err;
        }

        list_add_tail(&sl->w1_slave_entry, &sl->master->slist);
        sl->master->match_slave =1;
		//printf("__w1_attach_slave_device match \n");
        return 0;
}

int w1_attach_slave_device(struct w1_master *dev, struct w1_slave *sl,  struct w1_reg_num *rn)
{
        struct w1_family *f;
        int err;

        if (!sl) {
            return -1;
        }
        sl->master = dev;
        memcpy(&sl->reg_num, rn, sizeof(struct w1_reg_num));
		//printf("w1_attach_slave_device family %x\n",rn->family);
        f = w1_family_registered(rn->family);
        if (!f) {
            //printf("attach_slave family Id error or Not registered ! fail !! \n");
            return -1;
        }
        sl->family = f;
        //sl->reg_num.family =0x4B;                 /* only use test by wilbur */
        err = __w1_attach_slave_device(sl);
        if (err < 0) {
            return err;
        }
        dev->slave_count++;
        return 0;
}


int w1_slave_found(struct w1_master *dev, u64 rn)
{
        struct w1_slave *sl;
        struct w1_reg_num *tmp;
        int ret =0;
        //int i =0;
        //char * a;
        //for (i=0; i<8;i++) {
        //printf("rn %x\n",*((char*)&rn+i));
        //}
        //u64 rn_le = rn;
        tmp = (struct w1_reg_num *) &rn;

        //printf("fID %x,id %x,crc %x\n",tmp->family,tmp->id,tmp->crc);
        //printf("w1_slave_found !!!\n");

        sl = w1_slave_search_device(dev, tmp);
        if (sl) {

		    //printf("match one slave in slist\n");
            sl->master->match_slave =1;

            return ret;
        } else {
            //if (rn && tmp->crc == w1_calc_crc8((u8 *)&rn_le, 7)) {
            ret = w1_attach_slave_device(dev, &sl_satic, tmp);
            //printf("add new slave in slist\n");
            return ret;
            //}
        }

}

void w1_search_process_cb(struct w1_master *dev, u8 search_type,
	w1_slave_found_callback cb)
{
	w1_search_devices(dev, search_type, cb);

	if (dev->search_count > 0)
		dev->search_count--;
}

static void w1_search_process(struct w1_master *dev, u8 search_type)
{
	w1_search_process_cb(dev, search_type, w1_slave_found);
}


int w1_process(void *data)
{
	struct w1_master *dev = (struct w1_master *) data;
	/* As long as w1_timeout is only set by a module parameter the sleep
	 * time can be calculated in jiffies once.
	 */
	//const unsigned long jtime = msecs_to_jiffies(w1_timeout * 1000);
	/* remainder if it woke up early */
	//unsigned long jremain = 0;

	for (;;) {

            if (dev->search_count) {
                w1_search_process(dev, W1_SEARCH);
            }

            if (dev->match_slave ==1) {
                break;
            }

            if (!dev->search_count) {
                break;
            }
        }

	if (dev->match_slave == 1) {
		//printf("w1_process_match \n");
	    return 0;
	}
	else {
            return -1;
	}
}


int w1_register_family(struct w1_family *newf)
{
        list_h_t *ent, *n;
        struct w1_family *f;
        int ret = 0;

        list_for_each_safe(ent, n, &w1_families) {
            f  = (struct w1_family *)ent;
                if (f->fid == newf->fid) {
                    ret = -1;
                    break;
                }
        }

        if (!ret) {
            list_add_tail(&newf->family_entry, &w1_families);
			//printf("w1_register_family  add list \n");
        }

        //printf("w1_register_family  ok !!\n");
        return ret;
}


/*
 * Should be called under w1_flock held.
 */
struct w1_family * w1_family_registered(u8 fid)
{
	list_h_t *ent, *n;
	struct w1_family *f = NULL;
	int ret = 0;

	list_for_each_safe(ent, n, &w1_families) {
            f = (struct w1_family *)ent;
            if (f->fid == fid) {
                ret = 1;
                break;
            }
        }
        return (ret) ? f : NULL;
}

static inline void master_write_bit_dir(void *data, u8 bit)
{
        struct w1_gpio_platform_data *pdata = data;

        if (bit) {
            gpio_set_direction(pdata->pin,0);		  //input

        } else {
            gpio_set_direction(pdata->pin,1);		  //output
        }
}
/* this useful when read value by wilbur */
static u8 w1_gpio_read_bit(void *data)
{
        struct w1_gpio_platform_data *pdata = data;

        return gpio_get_data(pdata->pin) ? 1 : 0;
}

/* this useful when write value 1/0 by wilbur */
static void w1_gpio_write_bit(void *data, u8 bit)
{
        struct w1_gpio_platform_data *pdata = data;

	if (bit) {
            master_write_bit_dir(data, 0);
            udelay_mod(1);
            master_write_bit_dir(data, 1);
            udelay_mod(12);
	} else {                                                            //write 0 use this
            master_write_bit_dir(data, 0);                                  //output
            gpio_set_data(pdata->pin, 0);                                   // set 0

            udelay_mod(10);
            master_write_bit_dir(data, 1);                                  //input
            udelay_mod(6);
        }
}

/* this useful when write value 1/0 by wilbur */
static u8 w1_gpio_touch_bit(void *data, u8 bit)
{

        struct w1_gpio_platform_data *pdata = data;
        uint32_t ret =0;

        if (bit) {
            master_write_bit_dir(data, 0);         //output
            gpio_set_data(pdata->pin, 0);          //set 0
            udelay_mod(1);
            master_write_bit_dir(data, 1);         //input
            //udelay_mod(1);
            ret = w1_gpio_read_bit(data);
            udelay_mod(8);
            return ret;

        } else {
            w1_gpio_write_bit(data, bit);
            return 0;
        }
}


/* this useful when reset IC by wilbur */
static u8 w1_gpio_reset_bus(void *data)
{
        u8 result =0x0;
        struct w1_gpio_platform_data *pdata = data;
        master_write_bit_dir(data,0);                  //output
        gpio_set_data(pdata->pin, 0);                  //set 0
        udelay_mod(60);

        master_write_bit_dir(data,1);                  //input
        udelay_mod(7);

        result = w1_gpio_read_bit(data) & 0x1;

        udelay_mod(42);
        return result;
}

struct w1_master *w1_gpio_probe(struct w1_bus_master *bus_master, struct w1_gpio_platform_data *pdata)
{
        struct w1_master        *master_total;
		pdata->pin = 6; //gpio5(6)
		pdata->is_open_drain = 0;
        bus_master->data = pdata;
        bus_master->reset_bus =w1_gpio_reset_bus;
        bus_master->touch_bit = w1_gpio_touch_bit;
		//int read_value = 0;
		set_pin_function(pdata->pin);  //设置特定的pin为gpio模式,并且为输入模式;
        master_total = w1_add_master_device(bus_master);
        if (pdata->enable_external_pullup)
            pdata->enable_external_pullup(1);

        return master_total;
}


int w1_master_trigger_authentication(char*secret_buf,unsigned int cvalue)
{
        struct w1_slave *sl = NULL;
        struct w1_master *dev = master_total;
        sl = w1_slave_match_one(dev,0x4B);
        if (!sl) {
            printf("match slave fail \n");
            return -1;
        }
        return w1_ds28e1x_verifymac(sl,secret_buf,cvalue);
}

int w1_master_setup_slave(struct w1_master *dev, u8 fid, char*buf,char *w_buf)
{
        struct w1_slave *sl = NULL;
        sl = w1_slave_match_one(dev,fid);
        if (!sl) {
            printf("setup_slave match slave fail \n");
            return -1;
        }

        //return w1_ds28e1x_setup_device(sl,buf,w_buf);
        return 0;       /* in release version no need setup device so directly return 0 by wilbur  */
}


int w1_master_load_key(struct w1_master *dev, u8 fid, char*w_buf,char *r_buf)
{
        struct w1_slave *sl = NULL;
        sl = w1_slave_match_one(dev,fid);
        if (!sl) {
            printf("setup_slave match slave fail \n");
            return -1;
        }

        return w1_ds28e1x_write_read_key(sl,w_buf,r_buf);
}

int w1_master_auth_write_block_protection(struct w1_master *dev, u8 fid, uint8_t *key)
{
	int ret;
	struct w1_slave *sl = NULL;
	unsigned char id_buf[256], i = 0;
	unsigned char data[32] = {0};

	sl = w1_slave_match_one(dev, fid);
	if (!sl) {
		printf("match slave fail \n");
		return -1;
	}

	while (i < 50) {
		ret = w1_ds28e1x_read_status(sl, 0xE0, id_buf);
		if (ret == 0)
			break;

		udelay_mod(2*1000);
		i++;
	}

	if (ret == 0) {
		data[2] = id_buf[3];
		data[3] = id_buf[2];
	}

	memcpy(data + 4, key, 32);

	data[0] = 0x10;
	data[1] = 0x10;
	ret = w1_ds28e1x_write_authblockprotection(sl, data);
	if (ret != 0) {
	    printf("w1_ds28e1x_write_authblockprotection: %d\n", ret);
	    return -1;
	}

	data[0] = 0x11;
	data[1] = 0x11;
	ret = w1_ds28e1x_write_authblockprotection(sl, data);
	if (ret != 0) {
	    printf("w1_ds28e1x_write_authblockprotection: %d\n", ret);
	    return -1;
	}

	return 0;
}

int w1_master_set_write_auth_mode(struct w1_master *dev, u8 fid)
{
	int ret;
	struct w1_slave *sl = NULL;

	sl = w1_slave_match_one(dev, fid);
	if (!sl) {
		printf("match slave fail \n");
		return -1;
	}

	/* set block 0 Authentication Protection  */
	ret = w1_ds28e1x_write_blockprotection(sl, 0, PROT_BIT_AUTHWRITE_WRITE);
	if (ret != 0) {
		printf("write block 0 protect write fail: %d\n", ret);
		return -1;
	}

	udelay_mod(5*1000);

	/* set block 1 Authentication Protection  */
	ret =  w1_ds28e1x_write_blockprotection(sl, 1, PROT_BIT_AUTHWRITE_WRITE);
	if (ret != 0) {
		printf("write block 1 protect write fail:%d\n", ret);
		return -1;
	}

	udelay_mod(10*1000);

	return 0;
}

int w1_master_set_write_mode_for_test(struct w1_master *dev, u8 fid)
{
	int ret;
	struct w1_slave *sl = NULL;

	sl = w1_slave_match_one(dev, fid);
	if (!sl) {
		printf("match slave fail \n");
		return -1;
	}

	/* set block 0 Write Protection  */
	ret = w1_ds28e1x_write_blockprotection(sl, 0, PROT_BIT_WRITE_PROTECTION);
	if (ret != 0) {
		printf("write block 0 protect write fail: %d\n", ret);
		return -1;
	}

	udelay_mod(5*1000);

	/* set block 1 Write Protection  */
	ret =  w1_ds28e1x_write_blockprotection(sl, 1, PROT_BIT_WRITE_PROTECTION);
	if (ret != 0) {
		printf("write block 1 protect write fail:%d\n", ret);
		return -1;
	}

	udelay_mod(5*1000);

	/* reset block 0 Write Protection  */
	ret = w1_ds28e1x_write_blockprotection(sl, 0, PROT_BIT_RESET);
	if (ret != 0) {
		printf("write block 0 protect write fail: %d\n", ret);
		return -1;
	}

	udelay_mod(5*1000);

	/* reset block 1 Write Protection  */
	ret =  w1_ds28e1x_write_blockprotection(sl, 1, PROT_BIT_RESET);
	if (ret != 0) {
		printf("write block 1 protect write fail:%d\n", ret);
		return -1;
	}

	udelay_mod(5*1000);

	return 0;
}

int w1_master_get_write_auth_mode(struct w1_master *dev, u8 fid, unsigned char *buf)
{
	int ret, i = 0;
	struct w1_slave *sl = NULL;

	sl = w1_slave_match_one(dev, fid);
	if (!sl) {
		printf("match slave fail \n");
		return -1;
	}

	while (i < 50) {
		ret = w1_ds28e1x_read_status(sl, 0x0, buf);
		if (ret == 0)
			break;

		udelay_mod(2*1000);
		i++;
	}

	for (i = 0; i < 6; i++)
		printf("0x%x ", buf[i]);
	printf("\n");

	return 0;
}

int w1_master_is_write_auth_mode(struct w1_master *dev, u8 fid, int *yes)
{
	unsigned char buf[16];
	int ret;

	*yes = 0;
	ret = w1_master_get_write_auth_mode(dev, fid, buf);
	if (ret == 0) {
		if (buf[0] & 0x10 || buf[1] & 0x10)
			*yes = 1;
	}

	return ret;
}

int w1_master_auth_write_usr_mem(struct w1_master *dev, u8 fid,char* buf)
{
	uchar old_data[32];
	uchar data[10] = {0};
	uchar id_buf[256], manid[2];
	int ret = 0;
	int seg = 0;
	int i = 0;

	struct w1_slave *sl = NULL;
	//struct w1_master *dev = master_total;

	sl = w1_slave_match_one(dev, fid);
	if (!sl) {
		printf("match slave fail \n");
		return -1;
	}

	// read personality bytes to get manufacturer ID
	while (i < 50) {
		ret = w1_ds28e1x_read_status(sl, 0xE0, id_buf);
		if (ret == 0)
			break;

		udelay_mod(2*1000);
		i++;
	}

	if (ret == 0) {
		manid[0] = id_buf[3];
		manid[1] = id_buf[2];
	} else {
		printf("get man-id fail.\n");
		return -1;
	}

	udelay_mod(5*1000);

	ret = w1_ds28e1x_get_buffer(sl, old_data, 10);
	if (ret < 0) {
		printf("get old usr info fail.\n");
		return -1;
	}

	udelay_mod(5*1000);

	for (seg = 0; seg < 8; seg++) {
		memcpy(&data[0], &buf[seg * 4], 4);
		memcpy(&data[4], &old_data[seg * 4], 4);
		memcpy(&data[8], manid, 2);
		ret = w1_ds28e1x_write_authblock(sl, 0, seg, data, (seg == 0) ? 0 : 1);
		if (ret != 0) {
		    printf("w1 write authblock fail:%d\n", ret);
		    return -1;
		}
		udelay_mod(2*1000);
	}

	return 0;
}

int w1_master_get_rom_id(struct w1_master *dev, u8 fid, char* rom_id)
{
    struct w1_slave *sl = NULL;

    sl = w1_slave_match_one(dev, fid);
    if (!sl) {
        return -1;
    }

    if (rom_id)
        w1_ds28e1x_get_rom_id(rom_id);

    return 0;
}
