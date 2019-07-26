











































#include "hmac.h" 
#include "alloc.h"



debug_module_t mod_hmac = {
  0,                  
  "hmac sha-1"        
};


err_status_t
hmac_alloc(auth_t **a, int key_len, int out_len) {
  extern auth_type_t hmac;
  uint8_t *pointer;

  debug_print(mod_hmac, "allocating auth func with key length %d", key_len);
  debug_print(mod_hmac, "                          tag length %d", out_len);

  



  if (key_len > 20)
    return err_status_bad_param;

  
  if (out_len > 20)
    return err_status_bad_param;

  
  pointer = (uint8_t*)crypto_alloc(sizeof(hmac_ctx_t) + sizeof(auth_t));
  if (pointer == NULL)
    return err_status_alloc_fail;

  
  *a = (auth_t *)pointer;
  (*a)->type = &hmac;
  (*a)->state = pointer + sizeof(auth_t);  
  (*a)->out_len = out_len;
  (*a)->key_len = key_len;
  (*a)->prefix_len = 0;

  
  hmac.ref_count++;

  return err_status_ok;
}

err_status_t
hmac_dealloc(auth_t *a) {
  extern auth_type_t hmac;
  
  
  octet_string_set_to_zero((uint8_t *)a, 
			   sizeof(hmac_ctx_t) + sizeof(auth_t));

  
  crypto_free(a);
  
  
  hmac.ref_count--;

  return err_status_ok;
}

err_status_t
hmac_init(hmac_ctx_t *state, const uint8_t *key, int key_len) {
  int i;
  uint8_t ipad[64]; 
  
    



  if (key_len > 20)              
    return err_status_bad_param;
  
  



  for (i=0; i < key_len; i++) {    
    ipad[i] = key[i] ^ 0x36;
    state->opad[i] = key[i] ^ 0x5c;
  }  
  
  for (   ; i < 64; i++) {    
    ipad[i] = 0x36;
    ((uint8_t *)state->opad)[i] = 0x5c;
  }  

  debug_print(mod_hmac, "ipad: %s", octet_string_hex_string(ipad, 64));
  
  
  sha1_init(&state->init_ctx);

  
  sha1_update(&state->init_ctx, ipad, 64);
  memcpy(&state->ctx, &state->init_ctx, sizeof(sha1_ctx_t)); 

  return err_status_ok;
}

err_status_t
hmac_start(hmac_ctx_t *state) {
    
  memcpy(&state->ctx, &state->init_ctx, sizeof(sha1_ctx_t));

  return err_status_ok;
}

err_status_t
hmac_update(hmac_ctx_t *state, const uint8_t *message, int msg_octets) {

  debug_print(mod_hmac, "input: %s", 
	      octet_string_hex_string(message, msg_octets));
  
  
  sha1_update(&state->ctx, message, msg_octets);

  return err_status_ok;
}

err_status_t
hmac_compute(hmac_ctx_t *state, const void *message,
	     int msg_octets, int tag_len, uint8_t *result) {
  uint32_t hash_value[5];
  uint32_t H[5];
  int i;

  
  if (tag_len > 20)
    return err_status_bad_param;
  
  
  hmac_update(state, (const uint8_t*)message, msg_octets);
  sha1_final(&state->ctx, H);

  



  debug_print(mod_hmac, "intermediate state: %s", 
	      octet_string_hex_string((uint8_t *)H, 20));

  
  sha1_init(&state->ctx);
  
  
  sha1_update(&state->ctx, (uint8_t *)state->opad, 64);

  
  sha1_update(&state->ctx, (uint8_t *)H, 20);
  
  
  sha1_final(&state->ctx, hash_value);

  
  for (i=0; i < tag_len; i++)    
    result[i] = ((uint8_t *)hash_value)[i];

  debug_print(mod_hmac, "output: %s", 
	      octet_string_hex_string((uint8_t *)hash_value, tag_len));

  return err_status_ok;
}




uint8_t
hmac_test_case_0_key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
  0x0b, 0x0b, 0x0b, 0x0b
};

uint8_t 
hmac_test_case_0_data[8] = {
  0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65   
};

uint8_t
hmac_test_case_0_tag[20] = {
  0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 
  0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e, 
  0xf1, 0x46, 0xbe, 0x00
};

auth_test_case_t
hmac_test_case_0 = {
  20,                        
  hmac_test_case_0_key,      
  8,                          
  hmac_test_case_0_data,     
  20,                        
  hmac_test_case_0_tag,      
  NULL                       
};



char hmac_description[] = "hmac sha-1 authentication function";





auth_type_t
hmac  = {
  (auth_alloc_func)      hmac_alloc,
  (auth_dealloc_func)    hmac_dealloc,
  (auth_init_func)       hmac_init,
  (auth_compute_func)    hmac_compute,
  (auth_update_func)     hmac_update,
  (auth_start_func)      hmac_start,
  (char *)               hmac_description,
  (int)                  0,  
  (auth_test_case_t *)  &hmac_test_case_0,
  (debug_module_t *)    &mod_hmac,
  (auth_type_id_t)       HMAC_SHA1
};

