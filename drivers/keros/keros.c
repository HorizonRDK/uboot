/* SPDX-License-Identifier: GPL-2.0+
 *
 * Keros secure chip driver
 *
 * Copyright (C) 2020, Horizon Robotics, <yu.kong@horizon.ai>
 */

#include <common.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <edid.h>
#include <environment.h>
#include <errno.h>
#include <i2c.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include "keros/keros.h"
#include "keros/keros_lib_1_8v.h"

#define KEROS_PAGE_SIZE 30
#define I2C_MAX_OFFSET_LEN 4

static struct udevice *i2c_cur_bus;
// static uint8_t slave_addr = 0x1c;

static int keros_cmd_i2c_set_bus_num(unsigned int busnum)
{
    struct udevice *bus;
    int ret;

    ret = uclass_get_device_by_seq(UCLASS_I2C, busnum, &bus);
    if (ret) {
        debug("%s: No bus %d\n", __func__, busnum);
        return ret;
    }
    i2c_cur_bus = bus;

    return 0;
}

static int keros_i2c_get_cur_bus(struct udevice **busp)
{
#ifdef CONFIG_I2C_SET_DEFAULT_BUS_NUM
    if (!i2c_cur_bus) {
        if (keros_cmd_i2c_set_bus_num(CONFIG_I2C_DEFAULT_BUS_NUMBER)) {
            printf("Default I2C bus %d not found\n",
                   CONFIG_I2C_DEFAULT_BUS_NUMBER);
            return -ENODEV;
        }
    }
#endif

    if (!i2c_cur_bus) {
        puts("No I2C bus selected\n");
        return -ENODEV;
    }
    *busp = i2c_cur_bus;

    return 0;
}

static int keros_i2c_get_cur_bus_chip(uint chip_addr, struct udevice **devp)
{
    struct udevice *bus;
    int ret;

    ret = keros_i2c_get_cur_bus(&bus);
    if (ret)
        return ret;

    return i2c_get_chip(bus, chip_addr, 1, devp);
}

static uint8_t keros_i2c_report_err(int ret, int op)
{
    printf("Error %s the chip: %d\n",
           op == 0 ? "reading" : "writing", ret);

    return (uint8_t)CMD_RET_FAILURE;
}

int keros_interface_i2c_init(void)
{
    return keros_cmd_i2c_set_bus_num(0);
}

uint8_t I2CWrite(uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen,
                 uint8_t *pbData, uint8_t wDataLen)
{
    int32_t ret = 0;
    uint32_t offset = 0;
    struct udevice *dev;
    int32_t offset_len = wAddrLen;

    if (wAddrLen > I2C_MAX_OFFSET_LEN)
        return CMD_RET_FAILURE;
    while (offset_len)
        offset += (uint32_t)(*pbAddr++) << (8 * (wAddrLen - offset_len--));

    ret = keros_i2c_get_cur_bus_chip(bDevAddr, &dev);
    if (!ret && wAddrLen != -1)
        ret = i2c_set_chip_offset_len(dev, wAddrLen);
    if (ret)
        return keros_i2c_report_err(ret, 1);

    ret = dm_i2c_write(dev, offset, pbData, wDataLen);
    if (ret)
        return keros_i2c_report_err(ret, 1);
    return 0;
}


uint8_t I2CRead(uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen,
                uint8_t *pbData, uint8_t wDataLen)
{
    int32_t ret = 0;
    uint32_t offset = 0;
    struct udevice *dev;
    int32_t offset_len = wAddrLen;

    if (wAddrLen > I2C_MAX_OFFSET_LEN)
        return CMD_RET_FAILURE;
    while (offset_len)
        offset += (uint32_t)(*pbAddr++) << (8 * (wAddrLen - offset_len--));

    ret = keros_i2c_get_cur_bus_chip(bDevAddr, &dev);
    if (!ret && wAddrLen != -1)
        ret = i2c_set_chip_offset_len(dev, wAddrLen);
    if (ret)
        return keros_i2c_report_err(ret, 0);

    ret = dm_i2c_read(dev, offset, pbData, wDataLen);
    if (ret)
        return keros_i2c_report_err(ret, 0);
    return 0;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/* USER CODE BEGIN  */

uint8_t keros_read_data(uint16_t sub_addr, int read_len, uint8_t *r_data)
{
    uint8_t bSubAddress[2];

    bSubAddress[1] = sub_addr >> 8;
    bSubAddress[0] = sub_addr & 0xff;

    return I2CRead(KEROS_DEVID_ADDR, bSubAddress, 2, r_data, read_len);
}

uint8_t keros_write_data(uint16_t sub_addr, uint8_t *w_data, int write_len)
{
    uint8_t bSubAddress[2];

    bSubAddress[1] = sub_addr >> 8;
    bSubAddress[0] = sub_addr & 0xff;

    return I2CWrite(KEROS_DEVID_ADDR, bSubAddress, 2, w_data, write_len);
}

// 1ms
void keros_delay(uint32_t wait_time)
{
    udelay(1000 * wait_time);
}

uint8_t keros_power_on(void)
{
    uint8_t Data = 0xF0;
    return I2CWrite(KEROS_DEVID_ADDR, NULL, 0, &Data, 1);  // Power ON
}

int keros_init(void)
{
    int ret = 0;
    int i = 0;
    uint8_t keros_sn[5] = {0};
    keros_power_on();
    ret = keros_power_on();
    if (ret != 0) {
        printf("keros poweron failed\n");
        return -1;
    }
    keros_delay(10);
    ret = keros_init_1_8v(keros_sn);
    if (ret != KEROS_STATUS_OK) {
        printf("keros init failed\n");
        return -1;
    }
    for (i = 0; i < 5; ++i) {
        debug("0x%.2x", keros_sn[i]);
    }
    debug("\n");
    return 0;
}

int keros_authentication(void)
{
    int ret = FALSE;
    int i = 0;
    uint8_t indata[16];

    for (i = 0; i < 16; i++) {
        indata[i] = keros_random_1_8v();
        debug("%.2x", indata[i]);
    }
    debug("\n");

    ret = keros_authentication_1_8v(SET_AES_KEY_SIZE_256, 0, indata);
    if (ret == FALSE) {
        printf("keros_authentication failed\n");
        return -1;
    }
    debug("keros authentication success\n");
    return 0;
}

int keros_write_key(uint32_t password, uint8_t page, uint8_t *key,
                    uint8_t encrytion)
{
    int ret = 0;
    int i = 0;
    uint8_t match_key[EEPROM_BANK_LEN] = {0};

    ret = keros_eeprom_write_1_8v(password, page, key, encrytion);
    if (ret != KEROS_STATUS_OK)
    {
        printf("keros write eeprom failed, error code: %d\n", ret);
        return -1;
    }
    memset(match_key, 0, EEPROM_BANK_LEN);
    ret = keros_eeprom_read_1_8v(password, page, match_key, encrytion);
    if (ret != KEROS_STATUS_OK) {
        printf("keros read eeprom failed, error code: %d\n", ret);
        return -1;
    }
    ret = 0;
    for (i = 0; i < EEPROM_BANK_LEN; ++i) {
        if (key[i] != match_key[i])
            ret = -1;
        debug("%x", key[i]);
    }
    debug("\n");
    if (ret != 0) {
        printf("keros read data and write data match failed\n");
        return -1;
    }
    return 0;
}

int keros_pwchg(uint8_t page, int old_password, int new_password)
{
    int ret = 0;
    ret = keros_eeprom_pwchg_1_8v(page, old_password, new_password);
    if (ret != KEROS_STATUS_OK) {
        printf("keros page %d password change faild\n", page);
        return -1;
    }
    return 0;
}
