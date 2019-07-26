












































#include "alloc.h"

#include "crypto_kernel.h"



debug_module_t mod_crypto_kernel = {
  0,                  
  "crypto kernel"     
};





extern debug_module_t mod_auth;
extern debug_module_t mod_cipher;
extern debug_module_t mod_stat;
extern debug_module_t mod_alloc;



 

extern cipher_type_t null_cipher;
extern cipher_type_t aes_icm;
extern cipher_type_t aes_cbc;






extern auth_type_t null_auth;
extern auth_type_t hmac;



crypto_kernel_t
crypto_kernel = {
  crypto_kernel_state_insecure,    
  NULL,                            
  NULL,                            
  NULL                             
};

#define MAX_RNG_TRIALS 25

err_status_t
crypto_kernel_init() {
  err_status_t status;  

  
  if (crypto_kernel.state == crypto_kernel_state_secure) {
    
    



    return crypto_kernel_status(); 
  }

  
  status = err_reporting_init("crypto");
  if (status)
    return status;

  
  status = crypto_kernel_load_debug_module(&mod_crypto_kernel);
  if (status)
    return status;
  status = crypto_kernel_load_debug_module(&mod_auth);
  if (status)
    return status;
  status = crypto_kernel_load_debug_module(&mod_cipher);
  if (status)
    return status;
  status = crypto_kernel_load_debug_module(&mod_stat);
  if (status)
    return status;
  status = crypto_kernel_load_debug_module(&mod_alloc);
  if (status)
    return status;
  
  
  status = rand_source_init();
  if (status)
    return status;

    
  status = stat_test_rand_source_with_repetition(rand_source_get_octet_string, MAX_RNG_TRIALS);
  if (status)
    return status;

  
  status = ctr_prng_init(rand_source_get_octet_string);
  if (status)
    return status;

    
  status = stat_test_rand_source_with_repetition(ctr_prng_get_octet_string, MAX_RNG_TRIALS);
  if (status)
    return status;
 
  
  status = crypto_kernel_load_cipher_type(&null_cipher, NULL_CIPHER);
  if (status) 
    return status;
  status = crypto_kernel_load_cipher_type(&aes_icm, AES_ICM);
  if (status) 
    return status;
  status = crypto_kernel_load_cipher_type(&aes_cbc, AES_CBC);
  if (status) 
    return status;

  
  status = crypto_kernel_load_auth_type(&null_auth, NULL_AUTH);
  if (status)
    return status;
  status = crypto_kernel_load_auth_type(&hmac, HMAC_SHA1);
  if (status)
    return status;

  
  crypto_kernel.state = crypto_kernel_state_secure;

  return err_status_ok;
}

err_status_t
crypto_kernel_status() {
  err_status_t status;
  kernel_cipher_type_t  *ctype = crypto_kernel.cipher_type_list;
  kernel_auth_type_t    *atype = crypto_kernel.auth_type_list;
  kernel_debug_module_t *dm    = crypto_kernel.debug_module_list;

    
  printf("testing rand_source...");
  status = stat_test_rand_source_with_repetition(rand_source_get_octet_string, MAX_RNG_TRIALS);
  if (status) {
    printf("failed\n");
    crypto_kernel.state = crypto_kernel_state_insecure;
    return status;
  }  
  printf("passed\n");

  
  while(ctype != NULL) {
    printf("cipher: %s\n", ctype->cipher_type->description);
    printf("  instance count: %d\n", ctype->cipher_type->ref_count);
    printf("  self-test: ");
    status = cipher_type_self_test(ctype->cipher_type);
    if (status) {
      printf("failed with error code %d\n", status);
      exit(status);
    }
    printf("passed\n");
    ctype = ctype->next;
  }
  
  
  while(atype != NULL) {
    printf("auth func: %s\n", atype->auth_type->description);
    printf("  instance count: %d\n", atype->auth_type->ref_count);
    printf("  self-test: ");
    status = auth_type_self_test(atype->auth_type);
    if (status) {
      printf("failed with error code %d\n", status);
      exit(status);
    }
    printf("passed\n");
    atype = atype->next;
  }

  
  printf("debug modules loaded:\n");
  while (dm != NULL) {
    printf("  %s ", dm->mod->name);  
    if (dm->mod->on)
      printf("(on)\n");
    else
      printf("(off)\n");
    dm = dm->next;
  }

  return err_status_ok;
}

err_status_t
crypto_kernel_list_debug_modules() {
  kernel_debug_module_t *dm = crypto_kernel.debug_module_list;

  
  printf("debug modules loaded:\n");
  while (dm != NULL) {
    printf("  %s ", dm->mod->name);  
    if (dm->mod->on)
      printf("(on)\n");
    else
      printf("(off)\n");
    dm = dm->next;
  }

  return err_status_ok;
}

err_status_t
crypto_kernel_shutdown() {
  err_status_t status;

  



  
  while (crypto_kernel.cipher_type_list != NULL) {
    kernel_cipher_type_t *ctype = crypto_kernel.cipher_type_list;
    crypto_kernel.cipher_type_list = ctype->next;
    debug_print(mod_crypto_kernel, 
		"freeing memory for cipher %s", 
		ctype->cipher_type->description);
    crypto_free(ctype);
  }

  
  while (crypto_kernel.auth_type_list != NULL) {
     kernel_auth_type_t *atype = crypto_kernel.auth_type_list;
     crypto_kernel.auth_type_list = atype->next;
     debug_print(mod_crypto_kernel, 
		"freeing memory for authentication %s",
		atype->auth_type->description);
     crypto_free(atype);
  }

  
  while (crypto_kernel.debug_module_list != NULL) {
    kernel_debug_module_t *kdm = crypto_kernel.debug_module_list;
    crypto_kernel.debug_module_list = kdm->next;
    debug_print(mod_crypto_kernel, 
		"freeing memory for debug module %s", 
		kdm->mod->name);
    crypto_free(kdm);
  }

    status = rand_source_deinit();
  if (status)
    return status;

  
  crypto_kernel.state = crypto_kernel_state_insecure;
  
  return err_status_ok;
}

static inline err_status_t
crypto_kernel_do_load_cipher_type(cipher_type_t *new_ct, cipher_type_id_t id,
				  int replace) {
  kernel_cipher_type_t *ctype, *new_ctype;
  err_status_t status;

  
  if (new_ct == NULL)
    return err_status_bad_param;

  if (new_ct->id != id)
    return err_status_bad_param;

  
  status = cipher_type_self_test(new_ct);
  if (status) {
    return status;
  }

  
  ctype = crypto_kernel.cipher_type_list;
  while (ctype != NULL) {
    if (id == ctype->id) {
      if (!replace)
	return err_status_bad_param;
      status = cipher_type_test(new_ct, ctype->cipher_type->test_data);
      if (status)
	return status;
      new_ctype = ctype;
      break;
    }
    else if (new_ct == ctype->cipher_type)
      return err_status_bad_param;    
    ctype = ctype->next;
  }

  
  if (ctype == NULL) {
  
    new_ctype = (kernel_cipher_type_t *) crypto_alloc(sizeof(kernel_cipher_type_t));
    if (new_ctype == NULL)
      return err_status_alloc_fail;
    new_ctype->next = crypto_kernel.cipher_type_list;

    
    crypto_kernel.cipher_type_list = new_ctype;    
  }
    
  
  new_ctype->cipher_type = new_ct;
  new_ctype->id = id;

  
  if (new_ct->debug != NULL)
    crypto_kernel_load_debug_module(new_ct->debug);
  

  return err_status_ok;
}

err_status_t
crypto_kernel_load_cipher_type(cipher_type_t *new_ct, cipher_type_id_t id) {
  return crypto_kernel_do_load_cipher_type(new_ct, id, 0);
}

err_status_t
crypto_kernel_replace_cipher_type(cipher_type_t *new_ct, cipher_type_id_t id) {
  return crypto_kernel_do_load_cipher_type(new_ct, id, 1);
}

err_status_t
crypto_kernel_do_load_auth_type(auth_type_t *new_at, auth_type_id_t id,
				int replace) {
  kernel_auth_type_t *atype, *new_atype;
  err_status_t status;

  
  if (new_at == NULL)
    return err_status_bad_param;

  if (new_at->id != id)
    return err_status_bad_param;

  
  status = auth_type_self_test(new_at);
  if (status) {
    return status;
  }

  
  atype = crypto_kernel.auth_type_list;
  while (atype != NULL) {
    if (id == atype->id) {
      if (!replace)
	return err_status_bad_param;
      status = auth_type_test(new_at, atype->auth_type->test_data);
      if (status)
	return status;
      new_atype = atype;
      break;
    }
    else if (new_at == atype->auth_type)
      return err_status_bad_param;    
    atype = atype->next;
  }

  
  if (atype == NULL) {
    
    new_atype = (kernel_auth_type_t *)crypto_alloc(sizeof(kernel_auth_type_t));
    if (new_atype == NULL)
      return err_status_alloc_fail;

    new_atype->next = crypto_kernel.auth_type_list;
    
    crypto_kernel.auth_type_list = new_atype;
  }
    
  
  new_atype->auth_type = new_at;
  new_atype->id = id;

  
  if (new_at->debug != NULL)
    crypto_kernel_load_debug_module(new_at->debug);
  

  return err_status_ok;

}

err_status_t
crypto_kernel_load_auth_type(auth_type_t *new_at, auth_type_id_t id) {
  return crypto_kernel_do_load_auth_type(new_at, id, 0);
}

err_status_t
crypto_kernel_replace_auth_type(auth_type_t *new_at, auth_type_id_t id) {
  return crypto_kernel_do_load_auth_type(new_at, id, 1);
}


cipher_type_t *
crypto_kernel_get_cipher_type(cipher_type_id_t id) {
  kernel_cipher_type_t *ctype;
  
  
  ctype = crypto_kernel.cipher_type_list;
  while (ctype != NULL) {
    if (id == ctype->id)
      return ctype->cipher_type; 
    ctype = ctype->next;
  } 

  
  return NULL;
}


err_status_t
crypto_kernel_alloc_cipher(cipher_type_id_t id, 
			      cipher_pointer_t *cp, 
			      int key_len) {
  cipher_type_t *ct;

  



  if (crypto_kernel.state != crypto_kernel_state_secure)
    return err_status_init_fail;

  ct = crypto_kernel_get_cipher_type(id);
  if (!ct)
    return err_status_fail;
  
  return ((ct)->alloc(cp, key_len));
}



auth_type_t *
crypto_kernel_get_auth_type(auth_type_id_t id) {
  kernel_auth_type_t *atype;
  
  
  atype = crypto_kernel.auth_type_list;
  while (atype != NULL) {
    if (id == atype->id)
      return atype->auth_type; 
    atype = atype->next;
  } 

  
  return NULL;
}

err_status_t
crypto_kernel_alloc_auth(auth_type_id_t id, 
			 auth_pointer_t *ap, 
			 int key_len,
			 int tag_len) {
  auth_type_t *at;

  



  if (crypto_kernel.state != crypto_kernel_state_secure)
    return err_status_init_fail;

  at = crypto_kernel_get_auth_type(id);
  if (!at)
    return err_status_fail;
  
  return ((at)->alloc(ap, key_len, tag_len));
}

err_status_t
crypto_kernel_load_debug_module(debug_module_t *new_dm) {
  kernel_debug_module_t *kdm, *new;

  
  if (new_dm == NULL)
    return err_status_bad_param;

  
  kdm = crypto_kernel.debug_module_list;
  while (kdm != NULL) {
    if (strncmp(new_dm->name, kdm->mod->name, 64) == 0)
      return err_status_bad_param;    
    kdm = kdm->next;
  }

  
  
  new = (kernel_debug_module_t *)crypto_alloc(sizeof(kernel_debug_module_t));
  if (new == NULL)
    return err_status_alloc_fail;
    
  
  new->mod = new_dm;
  new->next = crypto_kernel.debug_module_list;

  
  crypto_kernel.debug_module_list = new;    

  return err_status_ok;
}

err_status_t
crypto_kernel_set_debug_module(char *name, int on) {
  kernel_debug_module_t *kdm;
  
  
  kdm = crypto_kernel.debug_module_list;
  while (kdm != NULL) {
    if (strncmp(name, kdm->mod->name, 64) == 0) {
      kdm->mod->on = on;
      return err_status_ok;
    }
    kdm = kdm->next;
  }

  return err_status_fail;
}

err_status_t
crypto_get_random(unsigned char *buffer, unsigned int length) {
  if (crypto_kernel.state == crypto_kernel_state_secure)
    return ctr_prng_get_octet_string(buffer, length);
  else
    return err_status_fail;
}
