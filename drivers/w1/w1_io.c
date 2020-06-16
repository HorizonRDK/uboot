/*
 *	w1_io.c
 *
 * Copyright (c) 2004 Evgeniy Polyakov <zbr@ioremap.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "w1/w1.h"
#include "linux/string.h"

static int w1_delay_parm = 1;

static u8 w1_crc8_table[] = {
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

extern int printf(const char *fmt, ...);

static void w1_delay(unsigned long tm)
{

    udelay_mod(tm * w1_delay_parm);

}

static void w1_write_bit(struct w1_master *dev, int bit);
static u8 w1_read_bit(struct w1_master *dev);

/**
 * w1_touch_bit() - Generates a write-0 or write-1 cycle and samples the level.
 * @dev:	the master device
 * @bit:	0 - write a 0, 1 -      write a 0 read the level
 */
static u8 w1_touch_bit(struct w1_master *dev, int bit)
{
	if (dev->bus_master->touch_bit)
		return dev->bus_master->touch_bit(dev->bus_master->data, bit);
	else if (bit)
		return w1_read_bit(dev);
	else {
		w1_write_bit(dev, 0);
		return 0;
	}
}

/**
 * w1_write_bit() - Generates a write-0 or write-1 cycle.
 * @dev:	the master device
 * @bit:	bit to write
 *
 * Only call if dev->bus_master->touch_bit is NULL
 */
static void w1_write_bit(struct w1_master *dev, int bit)
{
	if (bit) {
		dev->bus_master->write_bit(dev->bus_master->data, 0);
		w1_delay(1);
		dev->bus_master->write_bit(dev->bus_master->data, 1);
		w1_delay(13);
	} else {
		dev->bus_master->write_bit(dev->bus_master->data, 0);
		w1_delay(9);
		dev->bus_master->write_bit(dev->bus_master->data, 1);
		w1_delay(5);
	}
}

/**
 * w1_pre_write() - pre-write operations
 * @dev:	the master device
 *
 * Pre-write operation, currently only supporting strong pullups.
 * Program the hardware for a strong pullup, if one has been requested and
 * the hardware supports it.
 */
static void w1_pre_write(struct w1_master *dev)
{
	if (dev->pullup_duration &&
		dev->enable_pullup && dev->bus_master->set_pullup) {
		dev->bus_master->set_pullup(dev->bus_master->data,
			dev->pullup_duration);
	}
}

/**
 * w1_post_write() - post-write options
 * @dev:	the master device
 *
 * Post-write operation, currently only supporting strong pullups.
 * If a strong pullup was requested, clear it if the hardware supports
 * them, or execute the delay otherwise, in either case clear the request.
 */
static void w1_post_write(struct w1_master *dev)
{
	if (dev->pullup_duration) {
		if (dev->enable_pullup && dev->bus_master->set_pullup)
			dev->bus_master->set_pullup(dev->bus_master->data, 0);
		else
			udelay_mod((dev->pullup_duration) * 1000);
		dev->pullup_duration = 0;
	}
}

/**
 * w1_write_8() - Writes 8 bits.
 * @dev:	the master device
 * @byte:	the byte to write
 */
void w1_write_8(struct w1_master *dev, u8 byte)
{
	int i;

	if (dev->bus_master->write_byte) {
		w1_pre_write(dev);
		dev->bus_master->write_byte(dev->bus_master->data, byte);
	}
	else
		for (i = 0; i < 8; ++i) {
			if (i == 7)
				w1_pre_write(dev);
			w1_touch_bit(dev, (byte >> i) & 0x1);
		}
	w1_post_write(dev);
}

/**
 * w1_read_bit() - Generates a write-1 cycle and samples the level.
 * @dev:	the master device
 *
 * Only call if dev->bus_master->touch_bit is NULL
 */
static u8 w1_read_bit(struct w1_master *dev)
{
	int result;

	/* sample timing is critical here */
	dev->bus_master->write_bit(dev->bus_master->data, 0);
	//w1_delay(1);
	dev->bus_master->write_bit(dev->bus_master->data, 1);
	//!w1_delay(1);//no need

	result = dev->bus_master->read_bit(dev->bus_master->data);
	w1_delay(6);
	w1_delay(6);
	return result & 0x1;
}

/**
 * w1_triplet() - * Does a triplet - used for searching ROM addresses.
 * @dev:	the master device
 * @bdir:	the bit to write if both id_bit and comp_bit are 0
 *
 * Return bits:
 *  bit 0 = id_bit
 *  bit 1 = comp_bit
 *  bit 2 = dir_taken
 * If both bits 0 & 1 are set, the search should be restarted.
 *
 * Return:        bit fields - see above
 */
u8 w1_triplet(struct w1_master *dev, int bdir)
{
	if (dev->bus_master->triplet)
		return dev->bus_master->triplet(dev->bus_master->data, bdir);
	else {
		u8 id_bit   = w1_touch_bit(dev, 1);
		u8 comp_bit = w1_touch_bit(dev, 1);
		u8 retval;

		if (id_bit && comp_bit)
			return 0x03;  /* error */

		if (!id_bit && !comp_bit) {
			/* Both bits are valid, take the direction given */
			retval = bdir ? 0x04 : 0;
		} else {
			/* Only one bit is valid, take that direction */
			bdir = id_bit;
			retval = id_bit ? 0x05 : 0x02;
		}

		if (dev->bus_master->touch_bit)
			w1_touch_bit(dev, bdir);
		else
			w1_write_bit(dev, bdir);
		return retval;
	}
}

/**
 * w1_read_8() - Reads 8 bits.
 * @dev:	the master device
 *
 * Return:        the byte read
 */
u8 w1_read_8(struct w1_master *dev)
{
	int i;
	u8 res = 0;

	if (dev->bus_master->read_byte)
		res = dev->bus_master->read_byte(dev->bus_master->data);
	else
		for (i = 0; i < 8; ++i) {
			res |= (w1_touch_bit(dev,1) << i);
		}

	return res;
}


/**
 * w1_write_block() - Writes a series of bytes.
 * @dev:	the master device
 * @buf:	pointer to the data to write
 * @len:	the number of bytes to write
 */
void w1_write_block(struct w1_master *dev, const u8 *buf, int len)
{
	int i;

	if (dev->bus_master->write_block) {
		w1_pre_write(dev);
		dev->bus_master->write_block(dev->bus_master->data, buf, len);
	}
	else
		for (i = 0; i < len; ++i)
			w1_write_8(dev, buf[i]); /* calls w1_pre_write */
	w1_post_write(dev);
}


/**
 * w1_touch_block() - Touches a series of bytes.
 * @dev:	the master device
 * @buf:	pointer to the data to write
 * @len:	the number of bytes to write
 */
void w1_touch_block(struct w1_master *dev, u8 *buf, int len)
{
	int i, j;
	u8 tmp;

	for (i = 0; i < len; ++i) {
		tmp = 0;
		for (j = 0; j < 8; ++j) {
			if (j == 7)
				w1_pre_write(dev);
			tmp |= w1_touch_bit(dev, (buf[i] >> j) & 0x1) << j;
		}

		buf[i] = tmp;
	}
}

/**
 * w1_read_block() - Reads a series of bytes.
 * @dev:	the master device
 * @buf:	pointer to the buffer to fill
 * @len:	the number of bytes to read
 * Return:	the number of bytes read
 */
u8 w1_read_block(struct w1_master *dev, u8 *buf, int len)
{
	int i;
	u8 ret;

	if (dev->bus_master->read_block)
		ret = dev->bus_master->read_block(dev->bus_master->data, buf, len);
	else {
		for (i = 0; i < len; ++i) {
			buf[i] = w1_read_8(dev);
		}
		ret = len;
	}

	return ret;
}

/**
 * w1_reset_bus() - Issues a reset bus sequence.
 * @dev:	the master device
 * Return:	0=Device present, 1=No device present or error
 */
int w1_reset_bus(struct w1_master *dev)
{
	int result = 0;

	if (dev->bus_master->reset_bus) {
		result = dev->bus_master->reset_bus(dev->bus_master->data) & 0x1;
	}

	return result;
}

u8 w1_calc_crc8(u8 * data, int len)
{
	u8 crc = 0;

	while (len--)
		crc = w1_crc8_table[crc ^ *data++];

	return crc;
}

void w1_search_devices(struct w1_master *dev, u8 search_type, w1_slave_found_callback cb)
{
	u64 rn;
	int slave_count = 0;
	int last_device;

	rn = dev->search_id;
	last_device = 0;

	while ( !last_device && (slave_count++ < dev->max_slave_count) ) {
		rn = 0;

		/*
		 * Reset bus and all 1-wire device state machines
		 * so they can respond to our requests.
		 *
		 * Return 0 - device(s) present, 1 - no devices present.
		 */

		if (w1_reset_bus(dev)) {
			if (dev->search_count > 0)
				dev->search_count--;

			printf("w1_reset_bus  error !!\n");
			break;
		}

		/* Do fast search on single slave bus */
		if (dev->max_slave_count == 1) {
			int rv;
			w1_write_8(dev, W1_READ_ROM);
			rv = w1_read_block(dev, (u8 *)&rn, 8);

			if (rv == 8 && rn) {
				cb(dev, rn);
			}
		}
	}
}

/**
 * w1_reset_select_slave() - reset and select a slave
 * @sl:		the slave to select
 *
 * Resets the bus and then selects the slave by sending either a skip rom
 * or a rom match.  A skip rom is issued if there is only one device
 * registered on the bus.
 * The w1 master lock must be held.
 *
 * Return:	0=success, anything else=error
 */
int w1_reset_select_slave(struct w1_slave *sl)
{
	if (w1_reset_bus(sl->master))
            return -1;

	if (sl->master->slave_count == 1)
            w1_write_8(sl->master, W1_SKIP_ROM);
	else {
            u8 match[9] = {W1_MATCH_ROM, };
            u64 *p = (u64 *)(&sl->reg_num);
            u64 rn = *p;
            memcpy(&match[1], &rn, 8);
            w1_write_block(sl->master, match, 9);
	}
	return 0;
}
