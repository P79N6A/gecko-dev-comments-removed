













































#include "datatypes.h"
#include "null_cipher.h"
#include "alloc.h"



extern debug_module_t mod_cipher;

err_status_t
null_cipher_alloc(cipher_t **c, int key_len) {
  extern cipher_type_t null_cipher;
  uint8_t *pointer;
  
  debug_print(mod_cipher, 
	      "allocating cipher with key length %d", key_len);

  
  pointer = (uint8_t*)crypto_alloc(sizeof(null_cipher_ctx_t) + sizeof(cipher_t));
  if (pointer == NULL)
    return err_status_alloc_fail;

  
  *c = (cipher_t *)pointer;
  (*c)->type = &null_cipher;
  (*c)->state = pointer + sizeof(cipher_t);

  
  (*c)->key_len = key_len;

  
  null_cipher.ref_count++;
  
  return err_status_ok;
  
}

err_status_t
null_cipher_dealloc(cipher_t *c) {
  extern cipher_type_t null_cipher;

  
  octet_string_set_to_zero((uint8_t *)c, 
			   sizeof(null_cipher_ctx_t) + sizeof(cipher_t));

  
  crypto_free(c);

  
  null_cipher.ref_count--;
  
  return err_status_ok;
  
}

err_status_t
null_cipher_init(null_cipher_ctx_t *ctx, const uint8_t *key, int key_len) {

  debug_print(mod_cipher, "initializing null cipher", NULL);

  return err_status_ok;
}

err_status_t
null_cipher_set_iv(null_cipher_ctx_t *c, void *iv) { 
  return err_status_ok;
}

err_status_t
null_cipher_encrypt(null_cipher_ctx_t *c,
		    unsigned char *buf, unsigned int *bytes_to_encr) {
  return err_status_ok;
}

char 
null_cipher_description[] = "null cipher";

cipher_test_case_t  
null_cipher_test_0 = {
  0,                 
  NULL,              
  0,                 
  0,                 
  NULL,              
  0,                 
  NULL,              
  NULL               
};






cipher_type_t null_cipher = {
  (cipher_alloc_func_t)         null_cipher_alloc,
  (cipher_dealloc_func_t)       null_cipher_dealloc,
  (cipher_init_func_t)          null_cipher_init,
  (cipher_encrypt_func_t)       null_cipher_encrypt,
  (cipher_decrypt_func_t)       null_cipher_encrypt,
  (cipher_set_iv_func_t)        null_cipher_set_iv,
  (char *)                      null_cipher_description,
  (int)                         0,
  (cipher_test_case_t *)       &null_cipher_test_0,
  (debug_module_t *)            NULL,
  (cipher_type_id_t)            NULL_CIPHER
};

