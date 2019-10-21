#include <stdlib.h>
#include "w1/hobot_aes.h"
#include "log.h"

int hb_aes_encrypt(char* in, char* key, char* out, int len) {

        int i =15;
    if(!in || !key || !out) return 0;

    unsigned char iv[AES_BLOCK_SIZE] = {0x48, 0x6f, 0x62, 0x6f, 0x74, 0x20, 0x52, 0x6f, 0x62, 0x6f, 0x74, 0x69, 0x63, 0x73, 0x21, 0x24};  //加密的初始化向量

    unsigned char key1[16];
    for (i = 15; i > 0; i--) {
        key1[i] = (key[i-1] ^ 0x35);
    }
    key1[0] = key[15] ^ 0x53;

    AES_KEY aes;
    if(AES_set_encrypt_key(key1, 128, &aes) < 0)
        return 0;

    AES_cbc_encrypt((unsigned char*)in, (unsigned char*)out, len, &aes, iv, AES_ENCRYPT);

    return 1;
}
int hb_aes_decrypt(char* in, char* key, char* out, int len) {

	int i =15;

    if(!in || !key || !out) return 0;

    unsigned char iv[AES_BLOCK_SIZE] = {0x48, 0x6f, 0x62, 0x6f, 0x74, 0x20, 0x52, 0x6f, 0x62, 0x6f, 0x74, 0x69, 0x63, 0x73, 0x21, 0x24};  //加密的初始化向量

    unsigned char key1[16];
    for (i = 15; i > 0; i--) {
       key1[i] = (key[i-1] ^ 0x35);
    }
    key1[0] = key[15] ^ 0x53;

    AES_KEY aes;
    if(AES_set_decrypt_key(key1, 128, &aes) < 0)
        return 0;

    AES_cbc_encrypt((unsigned char*)in, (unsigned char*)out, len, &aes, iv, AES_DECRYPT);

    return 1;
}

int hb_encrypt_buffer(char* buffer, int len) {
    if(!buffer) return 0;

    unsigned char *out = (unsigned char *)malloc(len+AES_BLOCK_SIZE);

    unsigned char iv[AES_BLOCK_SIZE] = {0x48, 0x6f, 0x62, 0x6f, 0x74, 0x20, 0x52, 0x6f, 0x62, 0x6f, 0x74, 0x69, 0x63, 0x73, 0x21, 0x24};
    char key[AES_BLOCK_SIZE] = {0x3f, 0x48, 0x15, 0x16, 0x6f, 0xae, 0xd2, 0xa6, 0xe6, 0x27, 0x15, 0x69, 0x09, 0xcf, 0x7a, 0x3c};

    AES_KEY aes;
    if(AES_set_encrypt_key((unsigned char*)key, 128, &aes) < 0)
        return 0;

    AES_cbc_encrypt((unsigned char *)buffer, out, len, &aes, iv, AES_ENCRYPT);

    len = ((len+AES_BLOCK_SIZE-1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;

    memcpy(buffer, out, len);

    free(out);

    return len;
}

int hb_decrypt_buffer(char* buffer, int len) {
    if(!buffer) return 0;

    assert(len % AES_BLOCK_SIZE == 0);

    unsigned char *out = (unsigned char *)malloc(len+AES_BLOCK_SIZE);

    unsigned char iv[AES_BLOCK_SIZE] = {0x48, 0x6f, 0x62, 0x6f, 0x74, 0x20, 0x52, 0x6f, 0x62, 0x6f, 0x74, 0x69, 0x63, 0x73, 0x21, 0x24};
    char key[AES_BLOCK_SIZE] = {0x3f, 0x48, 0x15, 0x16, 0x6f, 0xae, 0xd2, 0xa6, 0xe6, 0x27, 0x15, 0x69, 0x09, 0xcf, 0x7a, 0x3c};

    AES_KEY aes;
    if(AES_set_decrypt_key((unsigned char*)key, 128, &aes) < 0)
        return 0;

    AES_cbc_encrypt((unsigned char*)buffer, out, len, &aes, iv, AES_DECRYPT);

    memcpy(buffer, out, len);

    free(out);

    return len;
}
/**
int aes_encrypt_default(char* in, char* key, char* out, int len) {
    if(!in || !key || !out) return 0;

    AES_KEY aes;
    if(AES_set_encrypt_key((unsigned char*)key, 128, &aes) < 0)
        return 0;

    //输入输出字符串够长，并且是AES_BLOCK_SIZE的整数倍，需要严格限制
    int en_len=0;
    while(en_len<len) {
        AES_encrypt((unsigned char*)in, (unsigned char*)out, &aes);
        in+=AES_BLOCK_SIZE;
        out+=AES_BLOCK_SIZE;
        en_len+=AES_BLOCK_SIZE;
    }

    return 1;
}
int aes_decrypt_default(char* in, char* key, char* out, int len) {
    if(!in || !key || !out) return 0;

    AES_KEY aes;
    if(AES_set_decrypt_key((unsigned char*)key, 128, &aes) < 0)
        return 0;

    int en_len=0;
    while(en_len<len) {
        AES_decrypt((unsigned char*)in, (unsigned char*)out, &aes);
        in+=AES_BLOCK_SIZE;
        out+=AES_BLOCK_SIZE;
        en_len+=AES_BLOCK_SIZE;
    }

    return 1;
}
*/
