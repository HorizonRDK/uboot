/*
 * (c) Copyright 2019.10.25
 */

#ifndef  INCLUDE_X2A_SPACC_H_
#define  INCLUDE_X2A_SPACC_H_	1

#include <common.h>

/********* Register Offsets **********/
#define SPACC_REG_IRQ_EN	 0x00000L
#define SPACC_REG_IRQ_STAT   0x00004L
#define SPACC_REG_IRQ_CTRL   0x00008L
#define SPACC_REG_FIFO_STAT  0x0000CL
#define SPACC_REG_SDMA_BRST_SZ 0x00010L

/* HSM specific */
#define SPACC_REG_HSM_CMD_REQ	0x00014L
#define SPACC_REG_HSM_CMD_GNT	0x00018L

#define SPACC_REG_SRC_PTR	0x00020L
#define SPACC_REG_DST_PTR	0x00024L
#define SPACC_REG_OFFSET	 0x00028L
#define SPACC_REG_PRE_AAD_LEN  0x0002CL
#define SPACC_REG_POST_AAD_LEN 0x00030L

#define SPACC_REG_PROC_LEN   0x00034L
#define SPACC_REG_ICV_LEN	0x00038L
#define SPACC_REG_ICV_OFFSET   0x0003CL
#define SPACC_REG_IV_OFFSET  0x00040L

#define SPACC_REG_SW_CTRL	0x00044L
#define SPACC_REG_AUX_INFO   0x00048L
#define SPACC_REG_CTRL	 0x0004CL

#define SPACC_REG_STAT_POP   0x00050L
#define SPACC_REG_STATUS	 0x00054L
#define SPACC_REG_STAT_WD_CTRL 0x00080L

#define SPACC_REG_KEY_SZ	 0x00100L

#define SPACC_REG_VIRTUAL_RQST	 0x00140L
#define SPACC_REG_VIRTUAL_ALLOC	0x00144L
#define SPACC_REG_VIRTUAL_PRIO	 0x00148L
#define SPACC_REG_VIRTUAL_RC4_KEY_RQST 0x00150L
#define SPACC_REG_VIRTUAL_RC4_KEY_GNT  0x00154L

#define SPACC_REG_ID	  0x00180L
#define SPACC_REG_CONFIG	0x00184L
#define SPACC_REG_CONFIG2	 0x00190L
#define SPACC_REG_HSM_VERSION   0x00188L

#define SPACC_REG_SECURE_CTRL  0x001C0L
#define SPACC_REG_SECURE_RELEASE 0x001C4

#define SPACC_REG_SK_LOAD	0x00200L
#define SPACC_REG_SK_STAT	0x00204L
#define SPACC_REG_SK_KEY	 0x00240L

#define SPACC_REG_HSM_CTX_CMD  0x00300L
#define SPACC_REG_HSM_CTX_STAT   0x00304L

/********* SRC_PTR Bitmasks **********/

#define SPACC_SRC_PTR_PTR	   0xFFFFFFF8

/********* DST_PTR Bitmasks **********/

#define SPACC_DST_PTR_PTR	   0xFFFFFFF8

#define CRYPTO_INVALID_ALG	   (-8)

#define CRYPTO_OK			(0)

#define SPACC_TEST 0

#define AES_IMG_SIZE	32
#define AES_128_KEY_SZ  16
#define AES_128_IV_SZ   16
#define HASH_IMG_SIZE   64
#define HASH_RESULT_SIZE  32
#define ROM_TEST_NUM	3
#define SPACC_ALIGN (64*1024)

enum crypto_modes {
	CRYPTO_MODE_NULL,

	CRYPTO_MODE_AES_ECB,
	CRYPTO_MODE_AES_CBC,
	CRYPTO_MODE_AES_CTR,
	CRYPTO_MODE_AES_CCM,
	CRYPTO_MODE_AES_GCM,
	CRYPTO_MODE_AES_CFB,
	CRYPTO_MODE_AES_OFB,
	CRYPTO_MODE_HASH_MD5,
	CRYPTO_MODE_HMAC_MD5,
	CRYPTO_MODE_HASH_SHA1,
	CRYPTO_MODE_HMAC_SHA1,
	CRYPTO_MODE_HASH_SHA224,
	CRYPTO_MODE_HMAC_SHA224,
	CRYPTO_MODE_HASH_SHA256,
	CRYPTO_MODE_HMAC_SHA256,
	CRYPTO_MODE_HASH_SHA384,
	CRYPTO_MODE_HMAC_SHA384,
	CRYPTO_MODE_HASH_SHA512,
	CRYPTO_MODE_HMAC_SHA512,
	CRYPTO_MODE_HASH_SHA512_224,
	CRYPTO_MODE_HMAC_SHA512_224,
	CRYPTO_MODE_HASH_SHA512_256,
	CRYPTO_MODE_HMAC_SHA512_256,

	CRYPTO_MODE_LAST
};

enum ehash {
	H_NULL = 0,
	H_MD5 = 1,
	H_SHA1 = 2,
	H_SHA224 = 3,
	H_SHA256 = 4,
	H_SHA384 = 5,
	H_SHA512 = 6,
	H_XCBC = 7,
	H_CMAC = 8,
	H_KF9 = 9,
	H_SNOW3G_UIA2 = 10,
	H_CRC32_I3E802_3 = 11,
	H_ZUC_UIA3 = 12,
	H_SHA512_224 = 13,
	H_SHA512_256 = 14,
	H_MICHAEL = 15,
	H_SHA3_224 = 16,
	H_SHA3_256 = 17,
	H_SHA3_384 = 18,
	H_SHA3_512 = 19,
	H_SHAKE128 = 20,
	H_SHAKE256 = 21,
	H_POLY1305 = 22,
	H_MAX
};

static inline void mmio_write_32(uint64_t addr, uint32_t value)
{
	*(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read_32(uint64_t addr)
{
	return *(volatile uint32_t*)addr;
}

/* initial api */
int spacc_init(void);

/* This api must been called after spacc_init */
int spacc_ex(uint64_t src, uint64_t dst, unsigned int src_len,
			 int cipher, int encrypt, int hash, const unsigned char *key,
			 unsigned keylen, const unsigned char *iv, unsigned ivlen);

#if SPACC_TEST
void spacc_test(void);
int spacc_aes_test(void);
int spacc_hash_test(void);
#endif

#endif // INCLUDE_X2A_SPACC_H_
