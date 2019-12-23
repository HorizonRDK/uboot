/*
 * (c) Copyright 2019.10.25
 */

#ifndef INCLUDE_X2A_CRYPT_H_
#define INCLUDE_X2A_CRYPT_H_	1

#define ALG_SUCCESS			0x00000000
#define BAD_PARAMETER	0xFFFF0001
#define ALGO_INVALID	0xFFFF0002
#define ADDR_INVALID	0xFFFF0003
#define LOAD_FAILURE	0xFFFF0004
#define SIGNATURE_INVALID	0xFFFF0005
#define CRYPT_FAILURE		0xFFFF0006
#define VERIFY_FAILURE		0xFFFF0007
#define VERIFT_VER_FAILURE	0xFFFF0008
#define VERIFY_GPT_FAILURE	0xFFFF0009
#define CAL_DIGEST_FAILURE	0xFFFF000A

#define ALG_RSA1024			0x0101
#define ALG_RSA2048			0x0102
#define ALG_RSA4096			0x0103

#define DEGIST_MD5_SIZE		16
#define DEGIST_SHA1_SIZE	20
#define DEGIST_SHA224_SIZE	28
#define DEGIST_SHA256_SIZE	32
#define DEGIST_SHA384_SIZE	48
#define DEGIST_SHA512_SIZE	64
#define SIGN_RSA1024_SIZE	128
#define SIGN_RSA2048_SIZE	256
#define SIGN_RSA4096_SIZE	512

#define RSA_SIGN_SIZE	256
#define AES_KEY_SIZE	16

#define KERNEL_MAX_ADDR 0x40000000

#define X2_CRYPT_TEST 0

int x2a_load_kernel_raw(uint32_t src_addr, uint32_t inlen,
	uint32_t dst_addr);

int x2a_load_kernel_file(char *partition_name,
	char *image_name, uint32_t dst_addr);

int x2a_hash_get_digest_size(uint32_t algo);

int x2a_sha256_calculate_hash(const uint8_t *in,
	uint32_t inlen, uint8_t *hash);

int x2a_aes_crypt_image(uint8_t *src_data,
	uint8_t *dst_data, uint32_t inlen, uint8_t *key,
	uint8_t *iv, bool enc);

int x2a_auth_kernel_img(uint32_t algo, uint8_t *key,
	uint32_t salt_len, uint8_t *msg, uint32_t msg_len,
	uint8_t *sig, uint32_t sig_len);

#if X2_CRYPT_TEST
void x2a_aes_test(void);

void x2a_hash_test(void);

void x2a_rsa_test(void);
#endif

#endif //INCLUDE_X2A_CRYPT_H_
