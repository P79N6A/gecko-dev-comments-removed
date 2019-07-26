












































#ifndef _AES_H
#define _AES_H

#include "config.h"

#include "datatypes.h"
#include "gf2_8.h"
#include "err.h"



typedef struct {
  v128_t round[15];
  int num_rounds;
} aes_expanded_key_t;

err_status_t
aes_expand_encryption_key(const uint8_t *key,
			  int key_len,
			  aes_expanded_key_t *expanded_key);

err_status_t
aes_expand_decryption_key(const uint8_t *key,
			  int key_len,
			  aes_expanded_key_t *expanded_key);

void
aes_encrypt(v128_t *plaintext, const aes_expanded_key_t *exp_key);

void
aes_decrypt(v128_t *plaintext, const aes_expanded_key_t *exp_key);

#if 0




void
aes_init_sbox(void);

void
aes_compute_tables(void);
#endif 

#endif 
