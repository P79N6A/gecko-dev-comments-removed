












































#include "prng.h"



x917_prng_t x917_prng;

err_status_t
x917_prng_init(rand_source_func_t random_source) {
  uint8_t tmp_key[16];
  err_status_t status;

  
  x917_prng.octet_count = 0;

  
  x917_prng.rand = random_source;
  
  
  status = random_source(tmp_key, 16);
  if (status) 
    return status;

  
  aes_expand_encryption_key(tmp_key, 16, &x917_prng.key);

  
  status = x917_prng.rand((uint8_t *)&x917_prng.state, 16);
  if (status) 
    return status;

  return err_status_ok;
}

err_status_t
x917_prng_get_octet_string(uint8_t *dest, uint32_t len) {
  uint32_t t;
  v128_t buffer;
  uint32_t i, tail_len;
  err_status_t status;

  




  if (x917_prng.octet_count > MAX_PRNG_OUT_LEN - len) {
    status = x917_prng_init(x917_prng.rand);    
    if (status)
      return status;
  }
  x917_prng.octet_count += len;
  
  
  t = (uint32_t)time(NULL);
  
  
  for (i=0; i < len/16; i++) {
    
    
    x917_prng.state.v32[0] ^= t; 
 
    
    v128_copy(&buffer, &x917_prng.state);

    
    aes_encrypt(&buffer, &x917_prng.key);
    
    
    *dest++ = buffer.v8[0];
    *dest++ = buffer.v8[1];
    *dest++ = buffer.v8[2];
    *dest++ = buffer.v8[3];
    *dest++ = buffer.v8[4];
    *dest++ = buffer.v8[5];
    *dest++ = buffer.v8[6];
    *dest++ = buffer.v8[7];
    *dest++ = buffer.v8[8];
    *dest++ = buffer.v8[9];
    *dest++ = buffer.v8[10];
    *dest++ = buffer.v8[11];
    *dest++ = buffer.v8[12];
    *dest++ = buffer.v8[13];
    *dest++ = buffer.v8[14];
    *dest++ = buffer.v8[15];

    
    buffer.v32[0] ^= t;

    
    aes_encrypt(&buffer, &x917_prng.key);

    
    v128_copy(&x917_prng.state, &buffer);
    
  }
  
  
  tail_len = len % 16;
  if (tail_len) {
    
    
    x917_prng.state.v32[0] ^= t; 
 
    
    v128_copy(&buffer, &x917_prng.state);

    
    aes_encrypt(&buffer, &x917_prng.key);

    
    for (i=0; i < tail_len; i++) {
      *dest++ = buffer.v8[i];
    }

    

    
    buffer.v32[0] ^= t;

    
    aes_encrypt(&buffer, &x917_prng.key);

    
    v128_copy(&x917_prng.state, &buffer);

  }
  
  return err_status_ok;
}

err_status_t
x917_prng_deinit(void) {
  
  return err_status_ok;  
}
