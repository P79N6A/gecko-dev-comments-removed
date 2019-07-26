











































#ifndef PRNG_H
#define PRNG_H

#include "rand_source.h"  
#include "aes.h"          
#include "aes_icm.h"      

#define MAX_PRNG_OUT_LEN 0xffffffffU





typedef struct {
  v128_t   state;          
  aes_expanded_key_t key;  
  uint32_t octet_count;    
  rand_source_func_t rand; 
} x917_prng_t;

err_status_t
x917_prng_init(rand_source_func_t random_source);

err_status_t
x917_prng_get_octet_string(uint8_t *dest, uint32_t len);






typedef struct {
  uint32_t octet_count;    
  aes_icm_ctx_t   state;   
  rand_source_func_t rand; 
} ctr_prng_t;

err_status_t
ctr_prng_init(rand_source_func_t random_source);

err_status_t
ctr_prng_get_octet_string(void *dest, uint32_t len);


#endif
