












































#include "prng.h"



ctr_prng_t ctr_prng;

err_status_t
ctr_prng_init(rand_source_func_t random_source) {
  uint8_t tmp_key[32];
  err_status_t status;

  
  ctr_prng.octet_count = 0;

  
  ctr_prng.rand = random_source;
  
  
  status = random_source(tmp_key, 32);
  if (status) 
    return status;

  
  status = aes_icm_context_init(&ctr_prng.state, tmp_key, 30);
  if (status) 
    return status;

  return err_status_ok;
}

err_status_t
ctr_prng_get_octet_string(void *dest, uint32_t len) {
  err_status_t status;

  




  if (ctr_prng.octet_count > MAX_PRNG_OUT_LEN - len) {
    status = ctr_prng_init(ctr_prng.rand);    
    if (status)
      return status;
  }
  ctr_prng.octet_count += len;

  


  status = aes_icm_output(&ctr_prng.state, (uint8_t*)dest, len);
  if (status)
    return status;
  
  return err_status_ok;
}

err_status_t
ctr_prng_deinit(void) {

  
  
  return err_status_ok;  
}
