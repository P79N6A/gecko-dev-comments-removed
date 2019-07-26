













































#include "aes_cbc.h"
#include "alloc.h"

debug_module_t mod_aes_cbc = {
  0,                 
  "aes cbc"          
};



err_status_t
aes_cbc_alloc(cipher_t **c, int key_len) {
  extern cipher_type_t aes_cbc;
  uint8_t *pointer;
  int tmp;

  debug_print(mod_aes_cbc, 
	      "allocating cipher with key length %d", key_len);

  if (key_len != 16 && key_len != 24 && key_len != 32)
    return err_status_bad_param;
  
  
  tmp = (sizeof(aes_cbc_ctx_t) + sizeof(cipher_t));
  pointer = (uint8_t*)crypto_alloc(tmp);
  if (pointer == NULL) 
    return err_status_alloc_fail;

  
  *c = (cipher_t *)pointer;
  (*c)->type = &aes_cbc;
  (*c)->state = pointer + sizeof(cipher_t);

  
  aes_cbc.ref_count++;

  
  (*c)->key_len = key_len;

  return err_status_ok;  
}

err_status_t
aes_cbc_dealloc(cipher_t *c) {
  extern cipher_type_t aes_cbc;

  
  octet_string_set_to_zero((uint8_t *)c, 
			   sizeof(aes_cbc_ctx_t) + sizeof(cipher_t));

  
  crypto_free(c);

  
  aes_cbc.ref_count--;
  
  return err_status_ok;  
}

err_status_t
aes_cbc_context_init(aes_cbc_ctx_t *c, const uint8_t *key, int key_len,
		     cipher_direction_t dir) {
  err_status_t status;

  debug_print(mod_aes_cbc, 
	      "key:  %s", octet_string_hex_string(key, key_len)); 

  
  switch (dir) {
  case (direction_encrypt):
    status = aes_expand_encryption_key(key, key_len, &c->expanded_key);
    if (status)
      return status;
    break;
  case (direction_decrypt):
    status = aes_expand_decryption_key(key, key_len, &c->expanded_key);
    if (status)
      return status;
    break;
  default:
    return err_status_bad_param;
  }


  return err_status_ok;
}


err_status_t
aes_cbc_set_iv(aes_cbc_ctx_t *c, void *iv) {
  int i;

  uint8_t *input = (uint8_t*) iv;
 
  
  for (i=0; i < 16; i++) 
    c->previous.v8[i] = c->state.v8[i] = input[i];

  debug_print(mod_aes_cbc, "setting iv: %s", v128_hex_string(&c->state)); 

  return err_status_ok;
}

err_status_t
aes_cbc_encrypt(aes_cbc_ctx_t *c,
		unsigned char *data, 
		unsigned int *bytes_in_data) {
  int i;
  unsigned char *input  = data;   
  unsigned char *output = data;   
  int bytes_to_encr = *bytes_in_data;

  


  if (*bytes_in_data & 0xf) 
    return err_status_bad_param;

  



  debug_print(mod_aes_cbc, "iv: %s", 
	      v128_hex_string(&c->state));
  
  



  while (bytes_to_encr > 0) {
    
    
    for (i=0; i < 16; i++)
      c->state.v8[i] ^= *input++;

    debug_print(mod_aes_cbc, "inblock:  %s", 
	      v128_hex_string(&c->state));

    aes_encrypt(&c->state, &c->expanded_key);

    debug_print(mod_aes_cbc, "outblock: %s", 
	      v128_hex_string(&c->state));

    
    for (i=0; i < 16; i++)
      *output++ = c->state.v8[i];

    bytes_to_encr -= 16;
  }

  return err_status_ok;
}

err_status_t
aes_cbc_decrypt(aes_cbc_ctx_t *c,
		unsigned char *data, 
		unsigned int *bytes_in_data) {
  int i;
  v128_t state, previous;
  unsigned char *input  = data;   
  unsigned char *output = data;   
  int bytes_to_encr = *bytes_in_data;
  uint8_t tmp;

  


  if (*bytes_in_data & 0x0f)
    return err_status_bad_param;    

  
  for (i=0; i < 16; i++) {
    previous.v8[i] = c->previous.v8[i];
  }

  debug_print(mod_aes_cbc, "iv: %s", 
	      v128_hex_string(&previous));
  
  



  while (bytes_to_encr > 0) {
    
    
    for (i=0; i < 16; i++) {
     state.v8[i] = *input++;
    }

    debug_print(mod_aes_cbc, "inblock:  %s", 
	      v128_hex_string(&state));
    
    
    aes_decrypt(&state, &c->expanded_key);

    debug_print(mod_aes_cbc, "outblock: %s", 
	      v128_hex_string(&state));

    




    for (i=0; i < 16; i++) {
      tmp = *output;
      *output++ = state.v8[i] ^ previous.v8[i];
      previous.v8[i] = tmp;
    }

    bytes_to_encr -= 16;
  }

  return err_status_ok;
}


err_status_t
aes_cbc_nist_encrypt(aes_cbc_ctx_t *c,
		     unsigned char *data, 
		     unsigned int *bytes_in_data) {
  int i;
  unsigned char *pad_start; 
  int num_pad_bytes;
  err_status_t status;

  



  num_pad_bytes = 16 - (*bytes_in_data & 0xf);
  pad_start = data;
  pad_start += *bytes_in_data;
  *pad_start++ = 0xa0;
  for (i=0; i < num_pad_bytes; i++) 
    *pad_start++ = 0x00;
   
  


  *bytes_in_data += num_pad_bytes;  

  


  status = aes_cbc_encrypt(c, data, bytes_in_data);
  if (status) 
    return status;

  return err_status_ok;
}


err_status_t
aes_cbc_nist_decrypt(aes_cbc_ctx_t *c,
		     unsigned char *data, 
		     unsigned int *bytes_in_data) {
  unsigned char *pad_end;
  int num_pad_bytes;
  err_status_t status;

  


  status = aes_cbc_decrypt(c, data, bytes_in_data);
  if (status) 
    return status;

  



  num_pad_bytes = 1;
  pad_end = data + (*bytes_in_data - 1);
  while (*pad_end != 0xa0) {   
    pad_end--;
    num_pad_bytes++;
  }
  
  
  *bytes_in_data -= num_pad_bytes;  

  return err_status_ok;
}


char 
aes_cbc_description[] = "aes cipher block chaining (cbc) mode";












uint8_t aes_cbc_test_case_0_key[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

uint8_t aes_cbc_test_case_0_plaintext[64] =  {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff 
};

uint8_t aes_cbc_test_case_0_ciphertext[80] = {
  0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 
  0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
  0x03, 0x35, 0xed, 0x27, 0x67, 0xf2, 0x6d, 0xf1, 
  0x64, 0x83, 0x2e, 0x23, 0x44, 0x38, 0x70, 0x8b

};

uint8_t aes_cbc_test_case_0_iv[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


cipher_test_case_t aes_cbc_test_case_0 = {
  16,                                    
  aes_cbc_test_case_0_key,               
  aes_cbc_test_case_0_iv,                
  16,                                    
  aes_cbc_test_case_0_plaintext,         
  32,                                    
  aes_cbc_test_case_0_ciphertext,        
  NULL                                   
};







uint8_t aes_cbc_test_case_1_key[16] = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
};

uint8_t aes_cbc_test_case_1_plaintext[64] =  {
  0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 
  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
  0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 
  0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
  0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
  0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
  0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 
  0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

uint8_t aes_cbc_test_case_1_ciphertext[80] = {
  0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
  0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
  0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
  0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
  0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
  0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16, 
  0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09, 
  0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7,
  0x39, 0x34, 0x07, 0x03, 0x36, 0xd0, 0x77, 0x99, 
  0xe0, 0xc4, 0x2f, 0xdd, 0xa8, 0xdf, 0x4c, 0xa3
};

uint8_t aes_cbc_test_case_1_iv[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

cipher_test_case_t aes_cbc_test_case_1 = {
  16,                                    
  aes_cbc_test_case_1_key,               
  aes_cbc_test_case_1_iv,                
  64,                                    
  aes_cbc_test_case_1_plaintext,         
  80,                                    
  aes_cbc_test_case_1_ciphertext,        
  &aes_cbc_test_case_0                    
};







uint8_t aes_cbc_test_case_2_key[32] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

uint8_t aes_cbc_test_case_2_plaintext[64] =  {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff 
};

uint8_t aes_cbc_test_case_2_ciphertext[80] = {
  0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
  0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89,  
  0x72, 0x72, 0x6e, 0xe7, 0x71, 0x39, 0xbf, 0x11,
  0xe5, 0x40, 0xe2, 0x7c, 0x54, 0x65, 0x1d, 0xee
};

uint8_t aes_cbc_test_case_2_iv[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

cipher_test_case_t aes_cbc_test_case_2 = {
  32,                                    
  aes_cbc_test_case_2_key,               
  aes_cbc_test_case_2_iv,                
  16,                                    
  aes_cbc_test_case_2_plaintext,         
  32,                                    
  aes_cbc_test_case_2_ciphertext,        
  &aes_cbc_test_case_1                   
};







uint8_t aes_cbc_test_case_3_key[32] = {
  0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
  0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
  0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
  0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t aes_cbc_test_case_3_plaintext[64] =  {
  0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 
  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
  0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 
  0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
  0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
  0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
  0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 
  0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

uint8_t aes_cbc_test_case_3_ciphertext[80] = {
  0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
  0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
  0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d,
  0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
  0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf,
  0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
  0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc,
  0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b,
  0xfb, 0x98, 0x20, 0x2c, 0x45, 0xb2, 0xe4, 0xa0,
  0x63, 0xc4, 0x68, 0xba, 0x84, 0x39, 0x16, 0x5a
};

uint8_t aes_cbc_test_case_3_iv[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

cipher_test_case_t aes_cbc_test_case_3 = {
  32,                                    
  aes_cbc_test_case_3_key,               
  aes_cbc_test_case_3_iv,                
  64,                                    
  aes_cbc_test_case_3_plaintext,         
  80,                                    
  aes_cbc_test_case_3_ciphertext,        
  &aes_cbc_test_case_2                    
};

cipher_type_t aes_cbc = {
  (cipher_alloc_func_t)          aes_cbc_alloc,
  (cipher_dealloc_func_t)        aes_cbc_dealloc,  
  (cipher_init_func_t)           aes_cbc_context_init,
  (cipher_encrypt_func_t)        aes_cbc_nist_encrypt,
  (cipher_decrypt_func_t)        aes_cbc_nist_decrypt,
  (cipher_set_iv_func_t)         aes_cbc_set_iv,
  (char *)                       aes_cbc_description,
  (int)                          0,   
  (cipher_test_case_t *)        &aes_cbc_test_case_3,
  (debug_module_t *)            &mod_aes_cbc,
  (cipher_type_id_t)             AES_CBC
};


