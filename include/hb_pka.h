/*
 * (c) Copyright 2019.10.25
 */

#ifndef INCLUDE_X2A_PKA_H_
#define INCLUDE_X2A_PKA_H_	1

#include <linux/types.h>

#define PKA_CTRL_GO		31
#define PKA_CTRL_STOP_RQST	 27
#define PKA_CTRL_M521_MODE	 16
#define PKA_CTRL_M521_MODE_BITS   5
#define PKA_CTRL_BASE_RADIX	 8
#define PKA_CTRL_BASE_RADIX_BITS  3
#define PKA_CTRL_PARTIAL_RADIX	0
#define PKA_CTRL_PARTIAL_RADIX_BITS 8

#define PKA_IRQ_EN_STAT 30
#define PKA_TEST 0

int PKA_public_decrypt(int flen, const unsigned char *src,
	const unsigned char *key, unsigned char *dest);
void pka_init(void);

#define HASH_SIZE 32
#define TEST_NUM 3
#define RSA_PUB_KEY_SIZE 256
#define RSA_SIGN_SIZE  256

#if PKA_TEST
void pka_test(void);
void pka_rsa_test(void);
#endif
#endif // INCLUDE_X2A_PKA_H_
