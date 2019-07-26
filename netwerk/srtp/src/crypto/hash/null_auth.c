














































#include "null_auth.h" 
#include "alloc.h"



extern debug_module_t mod_auth;

err_status_t
null_auth_alloc(auth_t **a, int key_len, int out_len) {
  extern auth_type_t null_auth;
  uint8_t *pointer;

  debug_print(mod_auth, "allocating auth func with key length %d", key_len);
  debug_print(mod_auth, "                          tag length %d", out_len);

  
  pointer = (uint8_t*)crypto_alloc(sizeof(null_auth_ctx_t) + sizeof(auth_t));
  if (pointer == NULL)
    return err_status_alloc_fail;

  
  *a = (auth_t *)pointer;
  (*a)->type = &null_auth;
  (*a)->state = pointer + sizeof (auth_t);  
  (*a)->out_len = out_len;
  (*a)->prefix_len = out_len;
  (*a)->key_len = key_len;

  
  null_auth.ref_count++;

  return err_status_ok;
}

err_status_t
null_auth_dealloc(auth_t *a) {
  extern auth_type_t null_auth;
  
  
  octet_string_set_to_zero((uint8_t *)a, 
			   sizeof(null_auth_ctx_t) + sizeof(auth_t));

  
  crypto_free(a);
  
  
  null_auth.ref_count--;

  return err_status_ok;
}

err_status_t
null_auth_init(null_auth_ctx_t *state, const uint8_t *key, int key_len) {

  
  
  return err_status_ok;
}

err_status_t
null_auth_compute(null_auth_ctx_t *state, uint8_t *message,
		   int msg_octets, int tag_len, uint8_t *result) {

  return err_status_ok;
}

err_status_t
null_auth_update(null_auth_ctx_t *state, uint8_t *message,
		   int msg_octets) {

  return err_status_ok;
}

err_status_t
null_auth_start(null_auth_ctx_t *state) {
  return err_status_ok;
}








auth_test_case_t
null_auth_test_case_0 = {
  0,                                       
  NULL,                                    
  0,                                        
  NULL,                                    
  0,                                       
  NULL,                                    
  NULL                                     
};



char null_auth_description[] = "null authentication function";

auth_type_t
null_auth  = {
  (auth_alloc_func)      null_auth_alloc,
  (auth_dealloc_func)    null_auth_dealloc,
  (auth_init_func)       null_auth_init,
  (auth_compute_func)    null_auth_compute,
  (auth_update_func)     null_auth_update,
  (auth_start_func)      null_auth_start,
  (char *)               null_auth_description,
  (int)                  0,  
  (auth_test_case_t *)   &null_auth_test_case_0,
  (debug_module_t *)     NULL,
  (auth_type_id_t)       NULL_AUTH
};

