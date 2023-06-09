/*
 * (c) Copyright 2019.10.25
 */

#include <hb_spacc.h>
#include <common.h>
#include <linux/string.h>

#if SPACC_TEST
unsigned char uboot_aes_test_key[ROM_TEST_NUM][AES_128_KEY_SZ] = {
	{0x00, 0x4f, 0xf8, 0x38, 0x24, 0x22, 0xe1, 0x55,
	 0x36, 0x35, 0xb0, 0xf1, 0x73, 0x5c, 0x34, 0xae, },
	{0x19, 0xe1, 0xbb, 0xd4, 0xc4, 0xdd, 0xe7, 0x5b,
	 0x4b, 0xa7, 0xe2, 0x30, 0xbf, 0xe9, 0xa2, 0xb6, },
	{0x73, 0x04, 0xc5, 0x7f, 0x57, 0x4c, 0x57, 0xf3,
	 0x6b, 0x14, 0x23, 0xd0, 0xeb, 0xbe, 0x53, 0x7f, },
};

unsigned char uboot_aes_test_iv[ROM_TEST_NUM][AES_128_IV_SZ] = {
	{0xb4, 0xc2, 0x36, 0xfa, 0x2e, 0xef, 0x17, 0x25,
	 0xdd, 0x05, 0x27, 0xfc, 0x23, 0xed, 0x8d, 0xbe, },
	{0x56, 0x3f, 0x89, 0x78, 0x64, 0x5c, 0x10, 0xca,
	 0x15, 0xba, 0xed, 0x33, 0x92, 0x85, 0xee, 0x0f, },
	{0x93, 0x53, 0x4c, 0x7b, 0x16, 0xcf, 0xdb, 0x76,
	 0x44, 0xef, 0x47, 0xa3, 0x7c, 0x31, 0xdb, 0xbd, },
};

unsigned char uboot_test_aes_img_enc[ROM_TEST_NUM][AES_IMG_SIZE] = {
	{0xff, 0xca, 0xfb, 0x91, 0xf7, 0x78, 0x23, 0x64,
	 0x23, 0x6d, 0x20, 0xf8, 0xe1, 0xab, 0x8a, 0x91,
	 0x48, 0x55, 0xf9, 0x3e, 0x40, 0x5b, 0xe5, 0x8e,
	 0xb0, 0x02, 0x77, 0xaa, 0x1d, 0xef, 0xe6, 0xf3, },
	{0x37, 0xfe, 0x67, 0xfe, 0x9b, 0x3a, 0x30, 0xe4,
	 0x4a, 0xd0, 0xc1, 0x8e, 0x6a, 0x90, 0xf6, 0x3f,
	 0x6c, 0x30, 0x87, 0x8a, 0x22, 0xc9, 0x08, 0xcc,
	 0x40, 0xea, 0x10, 0x61, 0xfc, 0xb2, 0x6b, 0x72, },
	{0xf0, 0x34, 0x3d, 0x43, 0x31, 0xd8, 0x2b, 0xda,
	 0x78, 0x07, 0xef, 0x35, 0x9d, 0xcb, 0x30, 0x82,
	 0x0f, 0xed, 0x7f, 0xf2, 0xb4, 0x57, 0x54, 0x34,
	 0x9a, 0xc5, 0xf4, 0xcf, 0x41, 0x98, 0x2d, 0xb6, },
};

unsigned char uboot_test_aes_img[AES_IMG_SIZE] = {
	0xd2, 0x38, 0x94, 0x0e, 0x38, 0x21, 0xbc, 0xed,
	0x24, 0xae, 0x0e, 0x55, 0xff, 0xb8, 0x27, 0x47,
	0x2e, 0x57, 0x7b, 0x7a, 0xdd, 0x46, 0xd8, 0xb9,
	0x42, 0x19, 0x1f, 0x0d, 0xea, 0x97, 0x4c, 0xff,
};

const unsigned char uboot_test_aes_dst[AES_IMG_SIZE] = { 0 };

unsigned char uboot_test_hash_img[ROM_TEST_NUM][HASH_IMG_SIZE] = {
	{0xc2, 0x39, 0x52, 0xd0, 0xc5, 0x6c, 0xd3, 0xc2, 0xe4, 0x3f, 0xf5, 0xbe,
	 0xcc, 0x20, 0xaf, 0xbf, 0x59, 0x10, 0xae, 0xda, 0xdd, 0xd2, 0x6f, 0xea,
	 0x07, 0x8e, 0x2b, 0x6f, 0x01, 0x67, 0x3d, 0x39, 0x42, 0x3f, 0xc8, 0x3c,
	 0x2d, 0x02, 0x12, 0x95, 0x60, 0xfe, 0x5b, 0x45, 0x19, 0x5e, 0xcf, 0xcc,
	 0x26, 0xcb, 0x76, 0xf5, 0x81, 0xf3, 0x51, 0xee, 0x7d, 0x2e, 0x8a, 0x62,
	 0x21, 0x6e, 0x2f, 0xf1, },
	{0xaa, 0x71, 0x40, 0x6d, 0xbc, 0x1f, 0xc8, 0x9f, 0x85, 0x43, 0xe7, 0x5d,
	 0x7a, 0x5d, 0x73, 0x54, 0x95, 0x5d, 0x3c, 0x9e, 0x7f, 0x12, 0x68, 0xe9,
	 0x26, 0x6a, 0x94, 0xf2, 0x99, 0x7e, 0x9b, 0x46, 0x0b, 0x8c, 0xad, 0xea,
	 0xc0, 0xa5, 0x43, 0x5d, 0x75, 0x38, 0x54, 0xe4, 0xd2, 0x70, 0x58, 0xb3,
	 0xfa, 0xaa, 0xc3, 0x44, 0x94, 0xc5, 0xf7, 0x49, 0x6e, 0xfa, 0x0f, 0x89,
	 0x74, 0xc5, 0x3c, 0x87, },
	{0x33, 0xea, 0x89, 0x3a, 0x04, 0xe9, 0x66, 0xdd, 0x25, 0xa6, 0xc5, 0xfd,
	 0xad, 0x01, 0x04, 0x9b, 0xaf, 0xe4, 0xa7, 0x4b, 0x24, 0x16, 0x4f, 0x8a,
	 0xa9, 0x82, 0x3a, 0xa0, 0x25, 0x06, 0x3a, 0x3c, 0x41, 0xd4, 0x28, 0x37,
	 0x0a, 0x61, 0xd1, 0x15, 0xf2, 0x45, 0x24, 0xba, 0x7d, 0x0b, 0xb3, 0x86,
	 0x4f, 0x00, 0x1d, 0xec, 0x39, 0x4a, 0xc9, 0xbf, 0x50, 0x13, 0x2f, 0xd5,
	 0x48, 0x01, 0x1e, 0x2d, },
};

unsigned char uboot_test_hash_res[ROM_TEST_NUM][HASH_RESULT_SIZE] = {
	{0x78, 0xba, 0xd2, 0x3d, 0x76, 0xab, 0x52, 0x75, 0xe4, 0x95, 0xf7, 0xda,
	 0xb6, 0xaf, 0x49, 0xe6, 0x0b, 0x57, 0x92, 0x76, 0xb8, 0x8d, 0xf3, 0x71,
	 0xd5, 0xaa, 0x24, 0x93, 0xe8, 0x07, 0xcc, 0x24, },
	{0xfd, 0x8d, 0xa5, 0xef, 0x42, 0xa0, 0x1e, 0x51, 0xd2, 0xbf, 0xe6, 0x26,
	 0x6f, 0x58, 0x33, 0x02, 0xad, 0xe0, 0xbc, 0x3d, 0xa9, 0x31, 0x53, 0xbb,
	 0xee, 0x45, 0x9a, 0xf7, 0x99, 0xea, 0x9d, 0x3e, },
	{0x4d, 0x13, 0xe4, 0x3f, 0xdb, 0xf9, 0xd0, 0x01, 0x1d, 0x9c, 0x18, 0x8e,
	 0x17, 0xd7, 0x51, 0x8a, 0xf7, 0xfd, 0xc6, 0x5d, 0x0d, 0xb1, 0xdd, 0xc9,
	 0x47, 0x23, 0xec, 0x39, 0x77, 0x32, 0x65, 0xd8, },
};

const unsigned char uboot_test_hash_dst[HASH_RESULT_SIZE] = { 0 };

int spacc_aes_test(void)
{
	int i = 0;

	printf("AES function rom test start (CBC with 128 key length)"
		"with 32byte image\n");

	for (i = 0; i < ROM_TEST_NUM; i++) {
		spacc_ex((uint64_t) &uboot_test_aes_img_enc[i][0],
				 (uint64_t) uboot_test_aes_dst, AES_IMG_SIZE,
				 CRYPTO_MODE_AES_CBC, 0, 0, &uboot_aes_test_key[i][0],
				 AES_128_KEY_SZ, &uboot_aes_test_iv[i][0], AES_128_IV_SZ);

		if (memcmp((void *) uboot_test_aes_img, (void *) uboot_test_aes_dst,
			 AES_IMG_SIZE)) {
			printf("Test fail at loop %i\n", i);

			return -1;
		}
	}

	printf("uboot aes test pass!\n");
	return 0;
}

int spacc_hash_test(void)
{
	int i = 0;

	printf("Hash function rom test start (sha256) with 64byte image\n");
	for (i = 0; i < ROM_TEST_NUM; i++) {
		spacc_ex((uint64_t) &uboot_test_hash_img[i][0],
				 (uint64_t) uboot_test_hash_dst, HASH_IMG_SIZE, 0, 1,
				 CRYPTO_MODE_HASH_SHA256, 0, 0, 0, 0);
		if (memcmp((void *) &uboot_test_hash_res[i][0], (void *) uboot_test_hash_dst,
			 HASH_RESULT_SIZE)) {
			printf("Test fail at loop %i\n", i);
			return -1;
		}
	}

	printf("uboot hash test pass!\n");
	return 0;
}
#endif
