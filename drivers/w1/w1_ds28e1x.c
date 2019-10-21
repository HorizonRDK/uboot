/*
 * 1-Wire implementation for the ds23el15 chip
 *
 * Copyright (C) 2013 maximintergrated
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <stdio.h>
#include <linux/string.h>
#include "w1/w1_ds28e1x.h"
#include "w1/w1.h"
#include "w1/w1_int.h"
#include "w1/w1_family.h"
#include "w1/w1_ds28e1x_sha256.h"
#define SANITY_DBGCHK 1

//!TBD: add this definition in w1_family.h will be better

#define W1_FAMILY_DS28E11        0x4B

// 1-Wire commands
#define CMD_WRITE_MEMORY         0x55
#define CMD_READ_MEMORY          0xF0
#define CMD_LOAD_LOCK_SECRET     0x33
#define CMD_COMPUTE_LOCK_SECRET  0x3C
#define CMD_SELECT_SECRET        0x0F
#define CMD_COMPUTE_PAGEMAC      0xA5
#define CMD_READ_STATUS          0xAA
#define CMD_WRITE_BLOCK_PROTECT  0xC3
#define CMD_WRITE_AUTH_MEMORY    0x5A
#define CMD_WRITE_AUTH_PROTECT   0xCC
#define CMD_PIO_READ             0xDD
#define CMD_PIO_WRITE            0x96
#define CMD_RELEASE		 0xAA

#define BLOCK_READ_PROTECT       0x80
#define BLOCK_WRITE_PROTECT      0x40
#define BLOCK_EPROM_PROTECT      0x20
#define BLOCK_WRITE_AUTH_PROTECT 0x10

#define ROM_CMD_SKIP             0x3C
#define ROM_CMD_RESUME           0xA5

#define SELECT_SKIP     0
#define SELECT_RESUME   1
#define SELECT_MATCH    2
#define SELECT_ODMATCH  3
#define SELECT_SEARCH   4
#define SELECT_READROM  5
#define SELECT_ODSKIP	6

#define PROT_BIT_AUTHWRITE	0x10
#define PROT_BIT_EPROM		0x20
#define PROT_BIT_WRITE		0x40
#define PROT_BIT_READ		0x80

#define DS28E15_FAMILY		0x17

#define DS28E15_PAGES		2	// 2 pages, 4 blocks, 7 segment in each page. total 64 bytes
#define PAGE_TO_BYTE		32	// 1 page = 32 bytes
#define BLOCK_TO_BYTE		16	// 1 block = 16 bytes
#define SEGMENT_TO_BYTE		4	// 1 segment = 4 bytes

//#define LOW_VOLTAGE
#ifdef LOW_VOLTAGE
#define SHA_COMPUTATION_DELAY    4
#define EEPROM_WRITE_DELAY       15
#define SECRET_EEPROM_DELAY      200
#else   //use this in 8909
#define SHA_COMPUTATION_DELAY    4
#define EEPROM_WRITE_DELAY       10
#define SECRET_EEPROM_DELAY      100//<-90
#endif

#define ID_MIN		0
#define ID_MAX		3
#define CO_MIN		0
#define CO_MAX		10
#define ID_DEFAULT	1
#define CO_DEFAULT	1
#define RETRY_LIMIT	50                  //retry cnt
#define RETRY_DELAY     2 //unit:ms
#define CPU_NUM_DS28E1X       3

#define AUTH_PAGE_NO    0

/* control dynamic write key and data to secure IC by wilbur */
//#define DYNAMIC_KEY     1


//!for production case. secret data will be calculated dynamiclly.
static u8 g_data_prd_secbase[32]={0x34,0x45,0x92,0x4a,0xcb,0x03,0x9a,0xbd,
								 0x72,0x14,0x56,0x78,0x8b,0x12,0x34,0xbc,
								 0x75,0x34,0x56,0x78,0x3a,0x12,0x41,0x4a,
								 0x32,0x3c,0x56,0x78,0x9b,0x12,0xb2,0xf2};
static u8 g_data_prd_secreal[32];//calculate from g_data_prd_secbase


//!partial base&challenge data for development.
//!for production g_data_partial_base should be different and keep confidental.
//!partial_ch data can be dynamic by seed and RNG

static u8 g_data_partial_ch[32]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
								0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,
								0x5F,0x37,0xF1,0x25,0x38,0x52,0x83,0x9E,
								0x7B,0xC9,0xD4,0x00,0xBC,0x47,0x27,0x5B};

// misc state
static unsigned short slave_crc16;

static int special_mode = 0;
static char special_values[2];
static char rom_no[8];

int verification = -1, id = 2, color, model, detect;


#define READ_EOP_BYTE(seg) (32-seg*4)


static u8 skip_setup  = 1;	// for test if the chip did not have secret code, we would need to write temp secret value
static u8 init_verify = 0;	// for inital verifying



//-----------------------------------------------------------------------------
// ------ DS28E15 Functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------
// Set or clear special mode flag
//
// 'enable'      - '1' to enable special mode or '0' to clear
//
void set_special_mode(int enable, uchar *values)
{
   special_mode= enable;
   special_values[0] = values[0];
   special_values[1] = values[1];
}

//--------------------------------------------------------------------------
// Calculate a new CRC16 from the input data shorteger.  Return the current
// CRC16 and also update the global variable CRC16.
//
static short oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

static unsigned short docrc16(unsigned short data)
{
	data = (data ^ (slave_crc16 & 0xff)) & 0xff;
	slave_crc16 >>= 8;

	if (oddparity[data & 0xf] ^ oddparity[data >> 4])
        slave_crc16 ^= 0xc001;

	data <<= 6;
	slave_crc16  ^= data;
	data <<= 1;
	slave_crc16   ^= data;

	return slave_crc16;
}

//--------------------------------------------------------------------------
//  Compute MAC to write a 4 byte memory block using an authenticated
//  write.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     segment - segment number in page (0 to 7)
//     new_data - 4 byte buffer containing the data to write
//     old_data - 4 byte buffer containing the data to write
//     manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//     mac - buffer to put the calculated mac into
//
//  Returns: TRUE - mac calculated
//           FALSE - Failed to calculate
//
int calculate_write_authMAC256(int page, int segment, char *new_data, char *old_data, char *manid,unsigned char *mac)
{
	unsigned char mt[64];

	// calculate MAC
	// clear
	memset(mt,0,64);

	// insert ROM number
	memcpy(&mt[32],rom_no,8);

	mt[43] = segment;
	mt[42] = page;
	mt[41] = manid[0];
	mt[40] = manid[1];

	// insert old data
	memcpy(&mt[44],old_data,4);

	// insert new data
	memcpy(&mt[48],new_data,4);

	// compute the mac
	return compute_mac256(mt, 55, &mac[0]);
}


/* Performs a Compute Next SHA-256 calculation given the provided 32-bytes
   of binding data and 8 byte partial secret. The first 8 bytes of the
   resulting MAC is set as the new secret.
//  Input Parameter:
    1. unsigned char *binding: 32 byte buffer containing the binding data
    2. unsigned char *partial: 8 byte buffer with new partial secret
    3. int page_nim: page number that the compute is calculated on
    4. unsigned char *manid: manufacturer ID
   Globals used : SECRET used in calculation and set to new secret
   Returns: TRUE if compute successful
            FALSE failed to do compute
*/
void  calculate_next_secret(unsigned char* binding, unsigned char* partial, int page_num, unsigned char* manid)
{
   unsigned char MT[128];
   unsigned char MAC[64];
   set_secret(g_data_prd_secbase);

   // clear
   memset(MT,0,128);

   // insert page data
   memcpy(&MT[0],binding,32);

   // insert challenge
   memcpy(&MT[32],partial,32);

   // insert ROM number or FF
   memcpy(&MT[96],rom_no,8);

   MT[106] = page_num;
   MT[105] = manid[0];
   MT[104] = manid[1];

   compute_mac256(MT, 119, MAC);

   // set the new secret to the first 32 bytes of MAC
   memcpy(g_data_prd_secreal, MAC, 32);
   set_secret(MAC);

}


//-----------------------------------------------------------------------------
// ------ DS28E15 Functions - 1 wire command
//-----------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  Write a 4 byte memory block. The block location is selected by the
//  page number and offset blcok within the page. Multiple blocks can
//  be programmed without re-selecting the device using the continue flag.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     segment - segment number in page (0 to 7)
//     data - 4 byte buffer containing the data to write
//     contflag - Flag to indicate the write is continued from the last (=1)
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_write_seg(struct w1_slave *sl, int page, int seg, uchar *data, int contflag)
{
	uchar buf[256],cs;
	int cnt, i, offset;
	int length =4;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -1;

	if (!contflag)
	{
		if (w1_reset_select_slave(sl)) {
			printf("w1_ds28e1x_write_seg reset  error !!\n");
			return -1;
		}

		buf[cnt++] = CMD_WRITE_MEMORY;
		buf[cnt++] = (seg << 5) | page;   // address

		// Send command
		w1_write_block(sl->master, &buf[0], 2);

		// Read CRC
		w1_read_block(sl->master, &buf[cnt], 2);

		cnt += 2;

		offset = cnt;
	}

	// add the data
	for (i = 0; i < length; i++)
		buf[cnt++] = data[i];

	// Send data
	w1_write_block(sl->master, data, length);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check the first CRC16
	if (!contflag)
	{
		slave_crc16 = 0;
		for (i = 0; i < offset; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001) {
			printf("w1_ds28e1x_write_seg  !contflag crc1  error !!\n");
			return -1;
		}
	}

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("w1_ds28e1x_write_seg crc1  error !!\n");
		return -1;
	}
	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for EEPROM writing.
	udelay_mod(EEPROM_WRITE_DELAY*1000);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;

}

//--------------------------------------------------------------------------
//  Write a memory segment. The segment location is selected by the
//  page number and offset segment within the page. Multiple segments can
//  be programmed without re-selecting the device.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     seg - segment number in page (0 to 7)
//     data - 4 byte multiple buffer containing the data to write
//     length - length to write (4 multiple number, 4, 8, ..., 32)
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_write_memory(struct w1_slave *sl, int seg, int page, uchar *data, int length)
{
	uchar buf[256];
	uchar cs=0;
	int i;

	if (!sl)
		return -1;

	// program one or more contiguous 4 byte segments of a memory block.
	if (length%4) {
		printf("w1_ds28e1x_write_memory  no align error !!\n");
		return -1;
	}


	memcpy(buf,data,length);

	for (i=0;i<length/4;i++) {
		cs = w1_ds28e1x_write_seg(sl, page, i, &buf[i*4], (i==0)? 0 : 1);
	}

    if(cs !=0) {
		printf("w1_ds28e1x_write_memory  error !!\n");
    }

	return cs;
}

//--------------------------------------------------------------------------
//  Write a 4 byte memory block using an authenticated write (with MAC).
//  The block location is selected by the
//  page number and offset blcok within the page. Multiple blocks can
//  be programmed without re-selecting the device using the continue flag.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     segment - segment number in page (0 to 7)
//     data - SHA input data for the Authenticated Write Memory including new_data, old_data, and manid
//	new_data - 4 byte buffer containing the data to write
//	old_data - 4 byte buffer containing the data to write
//	manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//
//  Restrictions
//     The memory block containing the targeted 4-byte segment must not be write protected.
//     The Read/Write Scratchpad command(w1_ds28e1x_write_scratchpad) must have been issued once
//     in write mode after power-on reset to ensure proper setup of the SHA-256 engine.
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_write_authblock(struct w1_slave *sl, int page, int segment, uchar *data, int contflag)
{
	uchar buf[256],cs;
	uchar new_data[4], old_data[4], manid[2];
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -1;

	memcpy(new_data, &data[0], 4);
	memcpy(old_data, &data[4], 4);
	memcpy(manid, &data[8], 2);

	if (!contflag) {
		if (w1_reset_select_slave(sl))
			return -1;

		buf[cnt++] = CMD_WRITE_AUTH_MEMORY;
		buf[cnt++] = (segment << 5) | page;   // address

		// Send command
		w1_write_block(sl->master, &buf[0], 2);

		// Read the first CRC
		w1_read_block(sl->master, &buf[cnt], 2);
		cnt += 2;

		offset = cnt;
	}

	// add the data
	for (i = 0; i < 4; i++)
		buf[cnt++] = new_data[i];

	// Send data - first 4bytes
	w1_write_block(sl->master, new_data, 4);

	// read the second CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// now wait for the MAC computation.
        udelay_mod(SHA_COMPUTATION_DELAY * 1000);

	if (!contflag) {
		// check the first CRC16
		slave_crc16 = 0;
		for (i = 0; i < offset; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001)
			return -1;
	}

	// check the second CRC16
	slave_crc16 = 0;

	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// compute the mac
	if (special_mode)
	{
		if (!calculate_write_authMAC256(page, segment, (char *)new_data, (char *)old_data, special_values, &buf[0]))
			return -1;
	}
	else
	{
		if (!calculate_write_authMAC256(page, segment, (char *)new_data, (char *)old_data, (char *)manid, &buf[0]))
			return -1;
	}

	// transmit MAC as a block - send the second 32bytes
	cnt = 0;
	w1_write_block(sl->master, buf, 32);

	// calculate CRC on MAC
	slave_crc16 = 0;
	for (i = 0; i < 32; i++)
		docrc16(buf[i]);

	// append read of CRC16 and CS byte
	w1_read_block(sl->master, &buf[0], 3);
	cnt = 3;

	// ckeck CRC16
	for (i = 0; i < (cnt - 1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt - 1] != 0xAA)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	udelay_mod(EEPROM_WRITE_DELAY * 1000);

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;

}

//--------------------------------------------------------------------------
//  Write a 4 byte memory block using an authenticated write (with MAC).
//  The MAC must be pre-calculated.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     segment - segment number in page (0 to 7)
//     new_data - 4 byte buffer containing the data to write
//     mac - mac to use for the write
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_write_authblockMAC(struct w1_slave *sl, int page, int segment, uchar *new_data, uchar *mac)
{
	uchar buf[256],cs;
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -1;

	// check if not continuing a previous block write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_AUTH_MEMORY;
	buf[cnt++] = (segment << 5) | page;   // address

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// add the data
	for (i = 0; i < 4; i++)
		buf[cnt++] = new_data[i];

	// Send data
	w1_write_block(sl->master, new_data, 4);

	// read first CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// now wait for the MAC computation.
        udelay_mod(SHA_COMPUTATION_DELAY*1000);

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check the second CRC16
	slave_crc16 = 0;

	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// transmit MAC as a block
	w1_write_block(sl->master, mac, 32);

	// calculate CRC on MAC
	slave_crc16 = 0;
	for (i = 0; i < 32; i++)
		docrc16(mac[i]);

	// append read of CRC16 and CS byte
	w1_read_block(sl->master, &buf[0], 3);
	cnt = 3;

	// ckeck CRC16
	for (i = 0; i < (cnt-1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt-1] != 0xAA)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	udelay_mod(EEPROM_WRITE_DELAY*1000);

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}

//--------------------------------------------------------------------------
//  Read memory. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//	 seg - segment number(0~7) in page
//     page - page number where the block to read is located (0 to 15)
//     rdbuf - 32 byte buffer to contain the data to read
//     length - length to read (allow jsut segment(4bytes) unit) (4, 8, 16, ... , 64)
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_read_memory(struct w1_slave *sl, int seg, int page, uchar *rdbuf, int length)
{
	uchar buf[256];
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl) {
		printf("ds28e1x_read_memory no slave error !!\n");
		return -1;
        }

	// Check presence detect
	if (w1_reset_select_slave(sl)) {
		printf("ds28e1x_read_memory reset slave error !!\n");
		return -1;
	}

	buf[cnt++] = CMD_READ_MEMORY;
	buf[cnt++] = (seg << 5) | page;	 // address

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// read data and CRC16
	w1_read_block(sl->master, &buf[cnt], length + 2);
	cnt += length + 2;

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_read_memory crc1 error !!\n");
		return -1;
	}

	if (READ_EOP_BYTE(seg) == length) {
		// check the second CRC16
		slave_crc16 = 0;


		for (i = offset; i < cnt; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001) {
			printf("ds28e1x_read_memory crc2 error !!\n");
			return -1;
		}
	}

	// copy the data to the read buffer
        memcpy(rdbuf, &buf[offset], length);

        for (i = 0; i < length; i++) {
            printf("memory  %x \n",buf[offset + i]);

        }
	return 0;
}


//--------------------------------------------------------------------------
//  Read page and verify CRC. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     rdbuf - 32 byte buffer to contain the data to read
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_read_page(struct w1_slave *sl, int page, uchar *rdbuf)
{
	return w1_ds28e1x_read_memory(sl, 0, page, rdbuf, 32);
}

//--------------------------------------------------------------------------
//  Write page and verify CRC. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//     page - page number where the block to write is located (0 to 1)
//     wtbuf - 32 byte buffer to contain the data to write
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28e1x_write_page(struct w1_slave *sl, int page, uchar *wtbuf)
{
	return w1_ds28e1x_write_memory(sl, 0, page, wtbuf, 32);
}


//----------------------------------------------------------------------
// Read the scratchpad (challenge or secret)
//
// 'rbbuf'      - 32 byte buffer to contain the data to read
//
// Return: 0 - select complete
//             else - error during select, device not present
//
int w1_ds28e1x_read_scratchpad(struct w1_slave *sl, uchar *rdbuf)
{
	uchar buf[256];
	int cnt=0, i, offset;

	if (!sl)
		return -1;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_SELECT_SECRET;
	buf[cnt++] = 0x0F;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// read data and CRC16
	w1_read_block(sl->master, &buf[cnt], 34);
	cnt+=34;

	// check first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
                printf("ds28e1x_read_scratchpad crc1 error !!\n");
		return -1;
        }

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
                printf("ds28e1x_read_scratchpad crc2 error !!\n");
		return -1;
        }

	// copy the data to the read buffer
	memcpy(rdbuf,&buf[offset],32);

	return 0;
}

//----------------------------------------------------------------------
// Write the scratchpad (challenge or secret)
//
// 'data'      - data to write to the scratchpad (32 bytes)
//
// Return: 0 - select complete
//             else - error during select, device not present
//
int w1_ds28e1x_write_scratchpad(struct w1_slave *sl, uchar *data)
{
	uchar buf[256];
	int cnt=0, i, offset;

	if (!sl)
		return -1;

	// select device for write
	if (w1_reset_select_slave(sl)) {
		printf("ds28e1x_write_scratchpad reset slave error !!\n");
		return -1;
	}

	buf[cnt++] = CMD_SELECT_SECRET;
	buf[cnt++] = 0x00;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// add the data
	memcpy(&buf[cnt], data, 32);
	cnt+=32;

	// Send the data
	w1_write_block(sl->master, data, 32);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_write_scratchpad crc1 error !!\n");
		return -2;
        }

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_write_scratchpad crc2 error !!\n");
		return -3;
        }

	return 0;
}

//----------------------------------------------------------------------
// Load first secret operation on the DS28E25/DS28E22/DS28E15.
//
// 'lock'      - option to lock the secret after the load (lock = 1)
//
// Restrictions
//             The Read/Write Scratchpad command(w1_ds28e1x_write_scratchpad) must have been issued
//             in write mode prior to Load and Lock Secret to define the secret's value.
//
// Return: 0 - load complete
//             else - error during load, device not present
//
int w1_ds28e1x_load_secret(struct w1_slave *sl, int lock)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -1;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_LOAD_LOCK_SECRET;
	buf[cnt++] = (lock) ? 0xE0 : 0x00;  // lock flag

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_load_secret crc error !!\n");
		return -1;
	}

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	// use 100ms for both E11&E15
	//TBD !!from data sheet. E11 is 100ms, E15 200ms(reference code using 100ms, and it works)
	udelay_mod(SECRET_EEPROM_DELAY*1000);

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA) {
		//printf("ds28e1x_load_secret cs ok!!\n");
		return 0;
	}
	else
		return cs;
}


//----------------------------------------------------------------------
// Compute secret operation on the DS28E25/DS28E22/DS28E15.
//
// 'partial'    - partial secret to load (32 bytes)
// 'page_num'   - page number to read 0 - 1
//  'lock'      - option to lock the secret after the load (lock = 1)
//
//  Restrictions
//     The Read/Write Scratchpad command(w1_ds28e1x_write_scratchpad) must have been issued
//     in write mode prior to Compute and Lock Secret to define the partial secret.
//
// Return: 0 - compute complete
//		  else - error during compute, device not present
//
int w1_ds28e1x_compute_secret(struct w1_slave *sl, int page_num, int lock)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -1;

	page_num = page_num & 0x01;
	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_COMPUTE_LOCK_SECRET;
	buf[cnt++] = (lock) ? (0xE0 | page_num) : page_num;  // lock flag

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	udelay_mod((SHA_COMPUTATION_DELAY * 2 + SECRET_EEPROM_DELAY)*1000);

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}
int returnCrossCheckresult(unsigned int wvalue)
{
	int i;
	unsigned int x,w;
	int odd_bit_sum = 0;
	x = wvalue;
	w = wvalue;
	for (i= 0; i < 32; i = i +  2) {
		odd_bit_sum += (x & 0x1);
		x = x >> 2;
	}
	w = w >> 1;                //ä¿è¯æœ€é«˜ä½æ˜¯0ï¼Œéœ€è¦åœ¨åº”ç”¨å±‚çš„é‡Œé¢å°†æœ€é«˜ä½è®¾ä¸ºèŠ¯ç‰‡éªŒè¯ç»“æžœï¼Œ0ä¸ºéªŒè¯é€šè¿‡ï¼Œ1ä¸ºéªŒè¯å¤±è´¥ã€‚
	w = w | odd_bit_sum;
    return w;
}

//--------------------------------------------------------------------------
//  Do Compute Page MAC command and return MAC. Optionally do
//  annonymous mode (anon != 0).
//
//  Parameters
//     pbyte - parameter byte including page_num and anon
//	page_num - page number to read 0, 1
//	anon - Flag to indicate Annonymous mode if (anon != 0)
//     mac - 32 byte buffer for page data read
//
//  Restrictions
//     The Read/Write Scratchpad command(w1_ds28e1x_write_scratchpad) must have been issued
//     in write mode prior to Compute and Read Page MAC to define the challenge.
//
//  Returns: 0 - page read has correct MAC
//               else - Failed to read page or incorrect MAC
//
int w1_ds28e1x_compute_read_pageMAC(struct w1_slave *sl, int pbyte, uchar *mac)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -1;

	// select device for write
	if (w1_reset_select_slave(sl)) {
		printf("ds28e1x_compute_read_pageMAC reset slave error !!\n");
		return -1;
        }

	buf[cnt++] = CMD_COMPUTE_PAGEMAC;
	buf[cnt++] = pbyte;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_compute_read_pageMAC crc1 error !!\n");
		return -2;
	}

	// now wait for the MAC computation.
        udelay_mod((SHA_COMPUTATION_DELAY * 2 *1000));

	// read the CS byte
	cs = w1_read_8(sl->master);
	if (cs != 0xAA) {
		printf("ds28e1x_compute_read_pageMAC cs error !!\n");
		return -3;
	}

	// read the MAC and CRC
	w1_read_block(sl->master, &buf[0], 34);

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < 34; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001) {
		printf("ds28e1x_compute_read_pageMAC crc2 error !!\n");
		return -4;
	}

	// copy MAC to return buffer
	memcpy(mac, buf, 32);

	return 0;
}

//--------------------------------------------------------------------------
//  Do Read Athenticated Page command and verify MAC. Optionally do
//  annonymous mode (anon != 0).
//
//  Parameters
//     page_num - page number to read 0, 1
//     challange - 32 byte buffer containing the challenge
//     mac - 32 byte buffer for mac read
//     page_data - 32 byte buffer to contain the data to read
//     manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//     skipread - Skip the read page and use the provided data in the 'page_data' buffer
//     anon - Flag to indicate Annonymous mode if (anon != 0)
//
//  Returns: 0 - page read has correct MAC
//               else - Failed to read page or incorrect MAC
//
int w1_ds28e1x_read_authverify(struct w1_slave *sl, int page_num, uchar *challenge, uchar *page_data, uchar *manid, int skipread, int anon, unsigned int w)
{
	uchar mac[32];
	uchar mt[128];
	int pbyte;
	int i = 0, rslt;

	if (!sl)
		return -1;

	// check to see if we skip the read (use page_data)
	if (!skipread)                     /* here skip read the page (binding data !) use ic eeprom fix value by wilbur*/
	//if (1)
	{
		// read the page to get data
		while (i < RETRY_LIMIT) {
			rslt = w1_ds28e1x_read_page(sl, page_num, page_data);
			if (rslt == 0) {
				printf("ds28e1x read authverify read page ok \n");
				break;
            }

			//mdelay(RETRY_DELAY); /* wait 10ms */
			udelay_mod((RETRY_DELAY*1000));
			i++;
		}
		if (i >= RETRY_LIMIT) {
			return -1;
		}
		i = 0;
	}

	// The Read/Write Scratch pad command must have been issued in write mode prior
	// to Compute and Read Page MAC to define the challenge
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28e1x_write_scratchpad(sl, challenge);
		if (rslt == 0) {
			printf("ds28e1x_write_scratchpad ok \n");
			break;
        }
		udelay_mod(RETRY_DELAY*1000);
		i++;
	}
	if (i >= RETRY_LIMIT) {
		printf("ds28e1x_read_authverify out retry error -2\n");
		return -2;
	}
	i = 0;

	// have device compute mac
	pbyte = anon ? 0xE0 : 0x00;
	pbyte = pbyte | page_num;
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28e1x_compute_read_pageMAC(sl, pbyte, mac);
		if (rslt == 0) {
			printf("ds28e1x_compute_read_pageMAC ok \n");
			break;
        }

		udelay_mod(RETRY_DELAY*1000);
		i++;
	}
	if (i >= RETRY_LIMIT) {
		printf("ds28e1x_compute_read_pageMAC out retry error -3\n");
		return -3;
	}
	// create buffer to compute and verify mac

	// clear
	memset(mt,0,128);

	// insert page data
    memcpy(&mt[0],page_data,32);

	// insert challenge
	memcpy(&mt[32],challenge,32);

	// insert ROM number or FF
	if (anon)
		memset(&mt[96],0xFF,8);
	else
		memcpy(&mt[96],rom_no,8);

	mt[106] = page_num;

	if (special_mode)
	{
		mt[105] = special_values[0];
		mt[104] = special_values[1];
	}
	else
	{
		mt[105] = manid[0];
		mt[104] = manid[1];
	}

	if (verify_mac256(mt, 119, mac) ==0) {
		printf("ds28e1x read authverify error return -4\n");
		return -4;
	} else {
		printf("ds28e1x read authverify ok !\n");
		return returnCrossCheckresult(w);
		//return 0;
	}
}


//--------------------------------------------------------------------------
//  Read status bytes, either personality or page protection.
//
//  Parameters
//     pbyte - include personality, allpages, and page_num
//	personality - flag to indicate the read is the 4 personality bytes (1)
//		      or page page protection (0)
//	allpages - flag to indicate if just one page (0) or all (1) page protection
//		      bytes.
//	block_num - block number if reading protection 0 to 3
//     rdbuf - 16 byte buffer personality bytes (length 4) or page protection
//            (length 1 or 16)
//
//  Returns: 0 - status read
//               else - Failed to read status
//
int w1_ds28e1x_read_status(struct w1_slave *sl, int pbyte, uchar *rdbuf)
{
	uchar buf[256];
	int cnt, i, offset,rdnum;
	int personality, allpages, block_num;

	if (!sl)
		return -1;

	personality = (pbyte & 0xE0) ? 1 : 0;
	allpages = (pbyte == 0) ? 1 : 0;
	block_num = pbyte & 0x03;

	cnt = 0;
	offset = 0;

	if (w1_reset_select_slave(sl)){
		printf("read_status  reset error !!\n");
		return -1;
	}

	buf[cnt++] = CMD_READ_STATUS;
	if (personality)
		buf[cnt++] = 0xE0;
	else if (!allpages)
		buf[cnt++] = block_num;
	else
		buf[cnt++] = 0;

	// send the command
	w1_write_block(sl->master, &buf[0], 2);
	offset = cnt + 2;

	// adjust data length
	if ((personality) || (allpages))
		rdnum = 8;
	else
		rdnum = 5;

	// Read the bytes
	w1_read_block(sl->master, &buf[cnt], rdnum);
	cnt += rdnum;

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001){
		printf("read_status  crc1 error !!\n");
		return -1;
	}

	if ((personality || allpages || (block_num == 1)))
	{
		// check the second CRC16
		slave_crc16 = 0;
		for (i = offset; i < cnt; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001){
			printf("read_status  crc2 error !!\n");
			return -1;
		}
	}

	// copy the data to the read buffer
	memcpy(rdbuf, &buf[offset], rdnum - 4);
	//printf("ds28e1x_read_status ok !!\n");
	return 0;
}

//--------------------------------------------------------------------------
//  Write page protection byte.
//
//  Parameters
//     block - block number (0 to 3) which covers two pages each
//     prot - protection byte
//
//  Returns: 0 - protection written
//               else - Failed to set protection
//
int w1_ds28e1x_write_blockprotection(struct w1_slave *sl, uchar block, uchar prot)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -1;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_BLOCK_PROTECT;

	// compute parameter byte
	buf[cnt++] = prot|block;

	w1_write_block(sl->master, &buf[0], cnt);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// sent release
	w1_write_8(sl->master, CMD_RELEASE);

	// now wait for programming
	udelay_mod(8 * 1000);

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}

//--------------------------------------------------------------------------
//  Write page protection byte.
//
//  Parameters
//     data - input data for Authenticated Write Block Protection
//	new_value - new protection byte(parameter byte) including block num
//	old_value - old protection byte(parameter byte)
//	manid - manufacturer ID
//
//  Returns: 0 - protection written
//               else - Failed to set protection
//
int w1_ds28e1x_write_authblockprotection(struct w1_slave *sl, uchar *data)
{
	uchar buf[256], cs, mt[64];
	int cnt=0, i;
	uchar new_value, old_value;
	uchar manid[2];

	if (!sl)
		return -1;

	new_value = data[0];
	old_value = data[1];
	manid[0] = data[2];
	manid[1] = data[3];

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_AUTH_PROTECT;
	buf[cnt++] = new_value;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// read first CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// now wait for the MAC computation.
	udelay_mod(SHA_COMPUTATION_DELAY * 1000);

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// calculate MAC
	// clear
	memset(mt, 0, 64);

	// insert secret
	memcpy(mt, data + 4, 32);

	// insert ROM number
	memcpy(&mt[32], rom_no, 8);

	// instert block and page
	mt[43] = 0;
	mt[42] = new_value & 0x0F;

	// check on special mode
	if (special_mode)
	{
		mt[41] = special_values[0];
		mt[40] = special_values[1];
	}
	else
	{
		mt[41] = manid[0];
		mt[40] = manid[1];
	}

	// old data
	mt[44] = (old_value & PROT_BIT_AUTHWRITE) ? 0x01 : 0x00;
	mt[45] = (old_value & PROT_BIT_EPROM) ? 0x01 : 0x00;
	mt[46] = (old_value & PROT_BIT_WRITE) ? 0x01 : 0x00;
	mt[47] = (old_value & PROT_BIT_READ) ? 0x01 : 0x00;
	// new data
	mt[48] = (new_value & PROT_BIT_AUTHWRITE) ? 0x01 : 0x00;
	mt[49] = (new_value & PROT_BIT_EPROM) ? 0x01 : 0x00;
	mt[50] = (new_value & PROT_BIT_WRITE) ? 0x01 : 0x00;
	mt[51] = (new_value & PROT_BIT_READ) ? 0x01 : 0x00;

	mt[52] = 0x80;
	mt[60] = 0xB8;

	// compute the mac
	compute_mac256(mt, 64, &buf[0]);
	cnt = 32;

	// send the MAC
	w1_write_block(sl->master, &buf[0], 32);

	// Read CRC and CS byte
	w1_read_block(sl->master, &buf[cnt], 3);
	cnt += 3;

	// ckeck CRC16
	slave_crc16 = 0;
	for (i = 0; i < (cnt - 1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt - 1] != 0xAA)
		return -1;

	// send release and strong pull-up
	// DATASHEET_CORRECTION - last bit in release is a read-zero so don't check echo of write byte
	w1_write_8(sl->master, 0xAA);

	// now wait for the MAC computation.
	udelay_mod(EEPROM_WRITE_DELAY * 1000);

	// read the CS byte
	cs = w1_read_8(sl->master);
	printf("<%s: %d>cs = 0x%x\n", __FUNCTION__, __LINE__, cs);
	if (cs == 0xAA)
		return 0;
	else
		return cs;
}


unsigned int d_calculateCrossCheckValue(unsigned int cvalue)
{
    unsigned int x, y, z, w, t ;
    x = 326334 + cvalue;
    y = 79 - cvalue;
    z = 54978;
    w = 99267659;
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    w = w ^ (w>>19) ^ t ^ (t>>8);
    return w;
}

int w1_ds28e1x_write_read_key(struct w1_slave *sl, char *secret_buf, char *r_buf)
{

    int rslt,rt;
    uchar master_data[32],master_secret[32];
    rt = 0;
    rslt = 0;

    if (secret_buf !=NULL) {

        memcpy(master_data, secret_buf, 32);
        memcpy(master_secret, &secret_buf[32], 32);

	//"Write scratchpad - master secret\n"
        rslt = w1_ds28e1x_write_scratchpad(sl, master_secret);

        if (rslt) {
            rt = -1;
            return rt;
        }

	set_secret(master_secret);
	set_romid((unsigned char *)rom_no);

         /* Load the master secret */
        rslt = w1_ds28e1x_load_secret(sl, 0);

        if (rslt) {
            rt = -1;
            return rt;
        }

#if 0   /* only write key to secure ic */
        /* load data here */
        w1_ds28e1x_write_page(sl, 0, master_data);

        // read personality bytes to get manufacturer ID
        while (i < RETRY_LIMIT) {
	      rslt = w1_ds28e1x_read_status(sl, 0xE0, buf);
	      if (rslt == 0) {
		  //printf("ds28e1x_read_status ok\n");
		  break;
	      }
	      udelay_mod(RETRY_DELAY*1000);
	      i++;
        }

#endif
        if (rslt == 0) {
	  //printf("w1_ds28e1x_write_read_key_ok\n",rt);
          return 0;
        } else {
	  rt = -1;
	  printf("w1_ds28e1x_write_read_key_error ->rt %d\n",rt);
	  return rt;
        }
    }

    printf("secret_buf null error\n");
    return -1;
}



//--------------------------------------------------------------------------
//  w1_ds28e1x_verifymac
//
//  compare two mac data, Device mac and calculated mac and
//  returned the result
//  Returns: 0 - Success, have the same mac data in both of Device and AP.
//               else - Failed, have the different mac data or verifying sequnce failed.
//
int w1_ds28e1x_verifymac(struct w1_slave *sl ,char*secret_buf,unsigned int cvalue)
{
	int rslt,rt;
	uchar buf[256], challenge[32], manid[2];
	uchar master_secret[32];
	uchar master_data[32];
	//uchar check_data[32];
        unsigned int w =0;
	rt = 0;
	rslt =0;
	int i = 0;

	/* write key and data to IC dynamic function only used in test version ,needs to be removel in release version by wilbur  */
#ifdef  DYNAMIC_KEY

    if (secret_buf !=NULL) {

        memcpy(master_data, secret_buf, 32);
        memcpy(master_secret, &secret_buf[32], 32);

	  // Store the master secret and romid.
		//"Write scratchpad - master secret\n"
	  rslt = w1_ds28e1x_write_scratchpad(sl, master_secret);

	  if (rslt) rt = -1;

	  /* Load the master secret */
	  rslt = w1_ds28e1x_load_secret(sl, 0);

	  if (rslt) rt = -1;

	  set_secret(master_secret);
	  set_romid(rom_no);
    }

      /* load data here modify by wilbur */
	  w1_ds28e1x_write_page(sl, 0, master_data);

#else
	 if (secret_buf !=NULL) {

	   memcpy(master_data, secret_buf, 32);
	   memcpy(master_secret, &secret_buf[32], 32);
	   set_secret(master_secret);
	   set_romid((unsigned char *)rom_no);
	}

#endif

	// read personality bytes to get manufacturer ID
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28e1x_read_status(sl, 0xE0, buf);
		if (rslt == 0) {
			printf("ds28e1x_read_status ok\n");
			break;
        }
		udelay_mod(RETRY_DELAY*1000);
		i++;
	}

	i = 0;

	if (rslt == 0)
	{
		manid[0] = buf[3];
		manid[1] = buf[2];
	} else {
		rt = -1;
		printf("w1_ds28e1x_read_status_error ->rt %d\n",rt);
		goto success;
	}

	memcpy(challenge, g_data_partial_ch,32);

    w = d_calculateCrossCheckValue(cvalue);

	rslt = w1_ds28e1x_read_authverify(sl, 0, challenge,master_data, manid, 1, 0,w);

    udelay_mod(RETRY_DELAY*1000);

	if (rslt) {
		rt = rslt;
		printf("w1_ds28e1x_verifymac ->rslt %d\n",rslt);
	}

success:
	return rt;
}

int w1_ds28e1x_setup_device(struct w1_slave *sl, char *secret_buf, char *binding)
{
	int rslt,rt;
	uchar buf[256];

	/* this write key and data function useless when in release version (fixed key and data) by wilbur  */
   #if  0

    if (secret_buf == NULL) {
        memcpy(master_secret, g_data_secret, 32); // Set temp_secret to the master secret
    }  else {
        memcpy(master_secret, secret_buf, 32);                      //load secret from buf
    }

	// ----- DS28EL25/DS28EL22/DS28E15/DS28E11 Setup
	rt = 0;

	//"Write scratchpad - master secret\n"
	rslt = w1_ds28e1x_write_scratchpad(sl, master_secret);
	if (rslt) rt = -1;

	//"Load the master secret\n"
	rslt = w1_ds28e1x_load_secret(sl, 0);
	if (rslt) rt = -1;


    /* removel the load data here ,load data when authverify begin modify by wilbur  */
    if (binding == NULL) {
    //"Load the binding page\n"
        rslt = w1_ds28e1x_write_page(sl, 0, g_data_binding_page);
    } else {
		rslt = w1_ds28e1x_write_page(sl, 0, binding);
    }

	if (rslt) rt = -1;

	//"Set the master secret in the Software SHA-256\n"

	set_secret(master_secret);
	set_romid(rom_no);

#endif

	// fill image with random data to write
	// read personality bytes to get manufacturer ID

	rslt = w1_ds28e1x_read_status(sl, 0xE0, buf);
	if (rslt != 0) {
		rt = -1;
		printf("ds28e1x_setup_device read_status error %d\n",rt);
		goto end;
	}

end:
	return rt;

}

//--------------------------------------------------------------------------
/*Read the 64-bit ROM ID of DS28E15
// Input parameter:
   1.RomID  :64Bits RomID Receiving Buffer

// Returns: 0     = success ,RomID CRC check is right ,RomID Sotred in RomID Buffer
            other = failure ,maybe on_reset() Error ,or CRC check Error;
*/
int w1_ds28e1x_read_romid(struct w1_slave *sl, unsigned char *RomID)
{
    int  i;
    u8 crc8;

    //reset
    if (w1_reset_bus(sl->master))
	return -1;

    if (sl->master->slave_count == 1)
        w1_write_8(sl->master, W1_READ_ROM);
    else
        return -1;

    udelay_mod(10);
    for(i = 0;i < 8;i++)
    {
        RomID[i] = w1_read_8(sl->master);
    }

    crc8 = w1_calc_crc8(RomID, 8);

    //if Receiving No  Error ,CRC =0;
    if(crc8!=0)
        return -2;
    else
        return 0;
}

void w1_ds28e1x_get_rom_id(char* rom_id) {
    if (rom_id)
        memcpy(rom_id, rom_no, 8);
}

/*
 w1 slave device node attribute directory layout is as follows:
 -->/sys/bus/w1/devices/
  devname_dir/          #device name. format: 17-xxxxxxxxxxxx (for E11 case)
    ©À©¤©¤ mode         #dev|prd. development<->debug. prd:production<->release.        {string}{RO}
    ©À©¤©¤ duid         #device unique id.(serial num of maxim chip).==devname_dir      {binary}{RO}
    ©À©¤©¤ seed         #seed for secure processing. (e.g. partial data / auth result)  {binary}{RW}
    ©À©¤©¤ mnid         #read device manufacture id                                     {binary}{RO}
    ©À©¤©¤ dmac         #read computed double SHA-256 MAC Result Data                   {binary}{RO}
    ©À©¤©¤ auth         #Authenticator MAC Verify result. in dev mode: 0:OK 1:NG        {binary}{RO}
    ©À©¤©¤ dbg_page     #page(0) data inject (mainly for debug in develoment)           {binary}{WO}
    ©¸©¤©¤ dbg_secret   #secert key data inject (mainly for debug in develoment)        {binary}{WO}

    PS: following attribute can be added for support for 2-level vendor lic case using E15
    ©À©¤©¤ ext_sid      #vendor specific sid if exist, default:0x00000000               {binary}{RW} suggest write-once.

 */



//static int w1_ds28e1x_get_buffer(struct w1_slave *sl, uchar *rdbuf, int retry_limit)
int w1_ds28e1x_get_buffer(struct w1_slave *sl, uchar *rdbuf, int retry_limit)
{
	int ret = -1, retry = 0;

	while((ret != 0) && (retry < retry_limit)) {
		ret = w1_ds28e1x_read_page(sl, 0, &rdbuf[0]);

		retry++;
	}

	return ret;
}

static int w1_ds28e1x_add_slave(struct w1_slave *sl)
{
	int err = 0;
        int ii;

        //printf("w1_ds28e1x_add_slave !!\n");
	// copy rom id to use mac calculation
	memcpy(rom_no, (u8 *)&sl->reg_num, sizeof(sl->reg_num));

        /* only add slave here get the ID info, no needs do verify and authentication by wilbur*/
	if (init_verify) {
		if (skip_setup == 0) {

			//in some race condition ofdevice search, setup may fail. so gurantee setup using re-try.
			ii = 0;
			while (ii < RETRY_LIMIT) {
				err = w1_ds28e1x_setup_device(sl,NULL,NULL);
				//err = w1_ds28e1x_setup_for_production(sl);
				if (err == 0)
					break;

				udelay_mod(10000);
				ii++;
			}

			if(err == 0)
				skip_setup = 1;
			err = w1_ds28e1x_verifymac(sl,NULL,0);
			verification = err;
		} else {
			err = w1_ds28e1x_verifymac(sl,NULL,0);
			verification = err;
		}
	}

	return err;
}

static struct w1_family_ops w1_ds28e1x_fops = {
	.add_slave      = w1_ds28e1x_add_slave,
};



static struct w1_family w1_ds28e11_family = {
	.fid = W1_FAMILY_DS28E11,
	.fops = &w1_ds28e1x_fops,
};

int w1_ds28e1x_init(void)
{
    int ret = 0;

    ret |= w1_register_family(&w1_ds28e11_family);
    return ret;
}
