












































#ifndef CRYPTO_KERNEL
#define CRYPTO_KERNEL

#include "rand_source.h"       
#include "prng.h"
#include "cipher.h"    
#include "auth.h"
#include "cryptoalg.h"
#include "stat.h"
#include "err.h"
#include "crypto_types.h"
#include "key.h"
#include "crypto.h"








typedef enum {
  crypto_kernel_state_insecure,
  crypto_kernel_state_secure
} crypto_kernel_state_t;





typedef struct kernel_cipher_type {
  cipher_type_id_t  id;
  cipher_type_t    *cipher_type;
  struct kernel_cipher_type *next;
} kernel_cipher_type_t;





typedef struct kernel_auth_type {
  auth_type_id_t  id;
  auth_type_t    *auth_type;
  struct kernel_auth_type *next;
} kernel_auth_type_t;





typedef struct kernel_debug_module {
  debug_module_t *mod;
  struct kernel_debug_module *next;
} kernel_debug_module_t;









typedef struct {
  crypto_kernel_state_t state;              
  kernel_cipher_type_t *cipher_type_list;   
  kernel_auth_type_t   *auth_type_list;     
  kernel_debug_module_t *debug_module_list; 
} crypto_kernel_t;



















err_status_t
crypto_kernel_init(void);













err_status_t
crypto_kernel_shutdown(void);











err_status_t
crypto_kernel_status(void);







err_status_t
crypto_kernel_list_debug_modules(void);






err_status_t
crypto_kernel_load_cipher_type(cipher_type_t *ct, cipher_type_id_t id);

err_status_t
crypto_kernel_load_auth_type(auth_type_t *ct, auth_type_id_t id);








err_status_t
crypto_kernel_replace_cipher_type(cipher_type_t *ct, cipher_type_id_t id);









err_status_t
crypto_kernel_replace_auth_type(auth_type_t *ct, auth_type_id_t id);


err_status_t
crypto_kernel_load_debug_module(debug_module_t *new_dm);












err_status_t
crypto_kernel_alloc_cipher(cipher_type_id_t id, 
			   cipher_pointer_t *cp, 
			   int key_len);













err_status_t
crypto_kernel_alloc_auth(auth_type_id_t id, 
			 auth_pointer_t *ap, 
			 int key_len,
			 int tag_len);











err_status_t
crypto_kernel_set_debug_module(char *mod_name, int v);





















err_status_t
crypto_get_random(unsigned char *buffer, unsigned int length);
     
#endif 
