













































#define ALIGN_32 0

#include "aes_icm.h"
#include "alloc.h"


debug_module_t mod_aes_icm = {
  0,                 
  "aes icm"          
};



































err_status_t
aes_icm_alloc_ismacryp(cipher_t **c, int key_len, int forIsmacryp) {
  extern cipher_type_t aes_icm;
  uint8_t *pointer;
  int tmp;

  debug_print(mod_aes_icm, 
            "allocating cipher with key length %d", key_len);

  







  if (!(forIsmacryp && key_len > 16 && key_len < 30) &&
      key_len != 30 && key_len != 38 && key_len != 46)
    return err_status_bad_param;

  
  tmp = (sizeof(aes_icm_ctx_t) + sizeof(cipher_t));
  pointer = (uint8_t*)crypto_alloc(tmp);
  if (pointer == NULL) 
    return err_status_alloc_fail;

  
  *c = (cipher_t *)pointer;
  (*c)->type = &aes_icm;
  (*c)->state = pointer + sizeof(cipher_t);

  
  aes_icm.ref_count++;

  
  (*c)->key_len = key_len;

  return err_status_ok;  
}

err_status_t aes_icm_alloc(cipher_t **c, int key_len, int forIsmacryp) {
  return aes_icm_alloc_ismacryp(c, key_len, 0);
}

err_status_t
aes_icm_dealloc(cipher_t *c) {
  extern cipher_type_t aes_icm;

  
  octet_string_set_to_zero((uint8_t *)c, 
			   sizeof(aes_icm_ctx_t) + sizeof(cipher_t));

  
  crypto_free(c);

  
  aes_icm.ref_count--;
  
  return err_status_ok;  
}












err_status_t
aes_icm_context_init(aes_icm_ctx_t *c, const uint8_t *key, int key_len) {
  err_status_t status;
  int base_key_len, copy_len;

  if (key_len > 16 && key_len < 30) 
    base_key_len = 16;
  else if (key_len == 30 || key_len == 38 || key_len == 46)
    base_key_len = key_len - 14;
  else
    return err_status_bad_param;

  



  v128_set_to_zero(&c->counter);
  v128_set_to_zero(&c->offset);

  copy_len = key_len - base_key_len;
  
  if (copy_len > 14)
    copy_len = 14;

  memcpy(&c->counter, key + base_key_len, copy_len);
  memcpy(&c->offset, key + base_key_len, copy_len);

  debug_print(mod_aes_icm, 
	      "key:  %s", octet_string_hex_string(key, base_key_len)); 
  debug_print(mod_aes_icm, 
	      "offset: %s", v128_hex_string(&c->offset)); 

  
  status = aes_expand_encryption_key(key, base_key_len, &c->expanded_key);
  if (status) {
    v128_set_to_zero(&c->counter);
    v128_set_to_zero(&c->offset);
    return status;
  }

  
  c->bytes_in_buffer = 0;

  return err_status_ok;
}







err_status_t
aes_icm_set_octet(aes_icm_ctx_t *c,
		  uint64_t octet_num) {

#ifdef NO_64BIT_MATH
  int tail_num       = low32(octet_num) & 0x0f;
  
  uint64_t block_num = make64(high32(octet_num) >> 4,
							  ((high32(octet_num) & 0x0f)<<(32-4)) |
							   (low32(octet_num) >> 4));
#else
  int tail_num       = (int)(octet_num % 16);
  uint64_t block_num = octet_num / 16;
#endif
  

  
  
  c->counter.v64[0] = c->offset.v64[0];
#ifdef NO_64BIT_MATH
  c->counter.v64[0] = make64(high32(c->offset.v64[0]) ^ high32(block_num),
							 low32(c->offset.v64[0])  ^ low32(block_num));
#else
  c->counter.v64[0] = c->offset.v64[0] ^ block_num;
#endif

  debug_print(mod_aes_icm, 
	      "set_octet: %s", v128_hex_string(&c->counter)); 

  
  if (tail_num) {
    v128_copy(&c->keystream_buffer, &c->counter);
    aes_encrypt(&c->keystream_buffer, &c->expanded_key);
    c->bytes_in_buffer = sizeof(v128_t);

    debug_print(mod_aes_icm, "counter:    %s", 
	      v128_hex_string(&c->counter));
    debug_print(mod_aes_icm, "ciphertext: %s", 
	      v128_hex_string(&c->keystream_buffer));    
    
    
    c->bytes_in_buffer = sizeof(v128_t) - tail_num;
  
  } else {
    
    
    c->bytes_in_buffer = 0;
  }

  return err_status_ok;
}






err_status_t
aes_icm_set_iv(aes_icm_ctx_t *c, void *iv) {
  v128_t *nonce = (v128_t *) iv;

  debug_print(mod_aes_icm, 
	      "setting iv: %s", v128_hex_string(nonce)); 
 
  v128_xor(&c->counter, &c->offset, nonce);
  
  debug_print(mod_aes_icm, 
	      "set_counter: %s", v128_hex_string(&c->counter)); 

  
  c->bytes_in_buffer = 0;

  return err_status_ok;
}









  
static inline void
aes_icm_advance_ismacryp(aes_icm_ctx_t *c, uint8_t forIsmacryp) {
  
  v128_copy(&c->keystream_buffer, &c->counter);
  aes_encrypt(&c->keystream_buffer, &c->expanded_key);
  c->bytes_in_buffer = sizeof(v128_t);

  debug_print(mod_aes_icm, "counter:    %s", 
	      v128_hex_string(&c->counter));
  debug_print(mod_aes_icm, "ciphertext: %s", 
	      v128_hex_string(&c->keystream_buffer));    
  
  

  if (forIsmacryp) {
    uint32_t temp;    
    
    temp = ntohl(c->counter.v32[3]);
    c->counter.v32[3] = htonl(++temp);
  } else {
    if (!++(c->counter.v8[15])) 
      ++(c->counter.v8[14]);
  }
}

static inline void aes_icm_advance(aes_icm_ctx_t *c) {
  aes_icm_advance_ismacryp(c, 0);
}















err_status_t
aes_icm_encrypt_ismacryp(aes_icm_ctx_t *c,
              unsigned char *buf, unsigned int *enc_len, 
              int forIsmacryp) {
  unsigned int bytes_to_encr = *enc_len;
  unsigned int i;
  uint32_t *b;

  
  if (!forIsmacryp && (bytes_to_encr + htons(c->counter.v16[7])) > 0xffff)
    return err_status_terminus;

 debug_print(mod_aes_icm, "block index: %d", 
           htons(c->counter.v16[7]));
  if (bytes_to_encr <= (unsigned int)c->bytes_in_buffer) {
    
    
    for (i = (sizeof(v128_t) - c->bytes_in_buffer);
		 i < (sizeof(v128_t) - c->bytes_in_buffer + bytes_to_encr); i++) 
	{
      *buf++ ^= c->keystream_buffer.v8[i];
	}

    c->bytes_in_buffer -= bytes_to_encr;

    
    return err_status_ok;

  } else {
    
        
    for (i=(sizeof(v128_t) - c->bytes_in_buffer); i < sizeof(v128_t); i++) 
      *buf++ ^= c->keystream_buffer.v8[i];

    bytes_to_encr -= c->bytes_in_buffer;
    c->bytes_in_buffer = 0;

  }
  
  
  for (i=0; i < (bytes_to_encr/sizeof(v128_t)); i++) {

    
    aes_icm_advance_ismacryp(c, forIsmacryp);

    




#if ALIGN_32
    b = (uint32_t *)buf;
    *b++ ^= c->keystream_buffer.v32[0];
    *b++ ^= c->keystream_buffer.v32[1];
    *b++ ^= c->keystream_buffer.v32[2];
    *b++ ^= c->keystream_buffer.v32[3];
    buf = (uint8_t *)b;
#else    
    if ((((unsigned long) buf) & 0x03) != 0) {
      *buf++ ^= c->keystream_buffer.v8[0];
      *buf++ ^= c->keystream_buffer.v8[1];
      *buf++ ^= c->keystream_buffer.v8[2];
      *buf++ ^= c->keystream_buffer.v8[3];
      *buf++ ^= c->keystream_buffer.v8[4];
      *buf++ ^= c->keystream_buffer.v8[5];
      *buf++ ^= c->keystream_buffer.v8[6];
      *buf++ ^= c->keystream_buffer.v8[7];
      *buf++ ^= c->keystream_buffer.v8[8];
      *buf++ ^= c->keystream_buffer.v8[9];
      *buf++ ^= c->keystream_buffer.v8[10];
      *buf++ ^= c->keystream_buffer.v8[11];
      *buf++ ^= c->keystream_buffer.v8[12];
      *buf++ ^= c->keystream_buffer.v8[13];
      *buf++ ^= c->keystream_buffer.v8[14];
      *buf++ ^= c->keystream_buffer.v8[15];
    } else {
      b = (uint32_t *)buf;
      *b++ ^= c->keystream_buffer.v32[0];
      *b++ ^= c->keystream_buffer.v32[1];
      *b++ ^= c->keystream_buffer.v32[2];
      *b++ ^= c->keystream_buffer.v32[3];
      buf = (uint8_t *)b;
    }
#endif 

  }
  
  
  if ((bytes_to_encr & 0xf) != 0) {
    
    
    aes_icm_advance_ismacryp(c, forIsmacryp);
    
    for (i=0; i < (bytes_to_encr & 0xf); i++)
      *buf++ ^= c->keystream_buffer.v8[i];
    
    
    c->bytes_in_buffer = sizeof(v128_t) - i;  
  } else {

    
    c->bytes_in_buffer = 0;

  }

  return err_status_ok;
}

err_status_t
aes_icm_encrypt(aes_icm_ctx_t *c, unsigned char *buf, unsigned int *enc_len) {
  return aes_icm_encrypt_ismacryp(c, buf, enc_len, 0);
}

err_status_t
aes_icm_output(aes_icm_ctx_t *c, uint8_t *buffer, int num_octets_to_output) {
  unsigned int len = num_octets_to_output;
  
  
  octet_string_set_to_zero(buffer, num_octets_to_output);
  
  
  return aes_icm_encrypt(c, buffer, &len);
}


char 
aes_icm_description[] = "aes integer counter mode";

uint8_t aes_icm_test_case_0_key[30] = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd
};

uint8_t aes_icm_test_case_0_nonce[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t aes_icm_test_case_0_plaintext[32] =  {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

uint8_t aes_icm_test_case_0_ciphertext[32] = {
  0xe0, 0x3e, 0xad, 0x09, 0x35, 0xc9, 0x5e, 0x80,
  0xe1, 0x66, 0xb1, 0x6d, 0xd9, 0x2b, 0x4e, 0xb4,
  0xd2, 0x35, 0x13, 0x16, 0x2b, 0x02, 0xd0, 0xf7,
  0x2a, 0x43, 0xa2, 0xfe, 0x4a, 0x5f, 0x97, 0xab
};

cipher_test_case_t aes_icm_test_case_0 = {
  30,                                    
  aes_icm_test_case_0_key,               
  aes_icm_test_case_0_nonce,             
  32,                                    
  aes_icm_test_case_0_plaintext,         
  32,                                    
  aes_icm_test_case_0_ciphertext,        
  NULL                                   
};

uint8_t aes_icm_test_case_1_key[46] = {
  0x57, 0xf8, 0x2f, 0xe3, 0x61, 0x3f, 0xd1, 0x70,
  0xa8, 0x5e, 0xc9, 0x3c, 0x40, 0xb1, 0xf0, 0x92,
  0x2e, 0xc4, 0xcb, 0x0d, 0xc0, 0x25, 0xb5, 0x82,
  0x72, 0x14, 0x7c, 0xc4, 0x38, 0x94, 0x4a, 0x98,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd
};

uint8_t aes_icm_test_case_1_nonce[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t aes_icm_test_case_1_plaintext[32] =  {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

uint8_t aes_icm_test_case_1_ciphertext[32] = {
  0x92, 0xbd, 0xd2, 0x8a, 0x93, 0xc3, 0xf5, 0x25,
  0x11, 0xc6, 0x77, 0xd0, 0x8b, 0x55, 0x15, 0xa4,
  0x9d, 0xa7, 0x1b, 0x23, 0x78, 0xa8, 0x54, 0xf6,
  0x70, 0x50, 0x75, 0x6d, 0xed, 0x16, 0x5b, 0xac
};

cipher_test_case_t aes_icm_test_case_1 = {
  46,                                    
  aes_icm_test_case_1_key,               
  aes_icm_test_case_1_nonce,             
  32,                                    
  aes_icm_test_case_1_plaintext,         
  32,                                    
  aes_icm_test_case_1_ciphertext,        
  &aes_icm_test_case_0                   
};







cipher_type_t aes_icm = {
  (cipher_alloc_func_t)          aes_icm_alloc,
  (cipher_dealloc_func_t)        aes_icm_dealloc,  
  (cipher_init_func_t)           aes_icm_context_init,
  (cipher_encrypt_func_t)        aes_icm_encrypt,
  (cipher_decrypt_func_t)        aes_icm_encrypt,
  (cipher_set_iv_func_t)         aes_icm_set_iv,
  (char *)                       aes_icm_description,
  (int)                          0,   
  (cipher_test_case_t *)        &aes_icm_test_case_1,
  (debug_module_t *)            &mod_aes_icm,
  (cipher_type_id_t)             AES_ICM
};

