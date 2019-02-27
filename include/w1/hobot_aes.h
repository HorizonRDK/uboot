#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_

#include "aes.h"
#define HOBOT_AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

int hb_aes_encrypt(char* in, char* key, char* out, int len);
int hb_aes_decrypt(char* in, char* key, char* out, int len);

int hb_encrypt_buffer(char* buffer, int len);
int hb_decrypt_buffer(char* buffer, int len);

#ifdef __cplusplus
}
#endif

#endif
