














































#ifndef NULL_CIPHER_H
#define NULL_CIPHER_H

#include "datatypes.h"
#include "cipher.h"

typedef struct {
  char foo ;
} null_cipher_ctx_t;







err_status_t
null_cipher_init(null_cipher_ctx_t *c, const uint8_t *key, int key_len);

err_status_t
null_cipher_set_segment(null_cipher_ctx_t *c,
			unsigned long segment_index);

err_status_t
null_cipher_encrypt(null_cipher_ctx_t *c,
		    unsigned char *buf, unsigned int *bytes_to_encr);


err_status_t
null_cipher_encrypt_aligned(null_cipher_ctx_t *c,
			    unsigned char *buf, int bytes_to_encr);

#endif 
