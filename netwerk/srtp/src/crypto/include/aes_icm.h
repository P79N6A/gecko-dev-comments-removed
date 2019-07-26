












































#ifndef AES_ICM_H
#define AES_ICM_H

#include "aes.h"
#include "cipher.h"

typedef struct {
  v128_t   counter;                
  v128_t   offset;                 
  v128_t   keystream_buffer;       
  aes_expanded_key_t expanded_key; 
  int      bytes_in_buffer;        
} aes_icm_ctx_t;


err_status_t
aes_icm_context_init(aes_icm_ctx_t *c,
		     const unsigned char *key,
		     int key_len); 

err_status_t
aes_icm_set_iv(aes_icm_ctx_t *c, void *iv);

err_status_t
aes_icm_encrypt(aes_icm_ctx_t *c,
		unsigned char *buf, unsigned int *bytes_to_encr);

err_status_t
aes_icm_output(aes_icm_ctx_t *c,
	       unsigned char *buf, int bytes_to_output);

err_status_t 
aes_icm_dealloc(cipher_t *c);
 
err_status_t 
aes_icm_encrypt_ismacryp(aes_icm_ctx_t *c, 
			 unsigned char *buf, 
			 unsigned int *enc_len, 
			 int forIsmacryp);
 
err_status_t 
aes_icm_alloc_ismacryp(cipher_t **c, 
		       int key_len, 
		       int forIsmacryp);

#endif 

