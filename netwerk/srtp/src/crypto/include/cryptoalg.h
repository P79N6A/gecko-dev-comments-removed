











































#ifndef CRYPTOALG_H
#define CRYPTOALG_H

#include "err.h"

































                    
typedef err_status_t (*cryptoalg_func_t) 
     (void *key,            
      const void *clear,          
      unsigned clear_len,   
      void *iv,             
      void *protect,         
      unsigned *protected_len);

typedef 
err_status_t (*cryptoalg_inv_t)
     (void *key,            
      const void *clear,     
      unsigned clear_len,   
      void *iv,             
      void *opaque,         
      unsigned *opaque_len  


      );

typedef struct cryptoalg_ctx_t {
  cryptoalg_func_t enc;
  cryptoalg_inv_t  dec;
  unsigned key_len;
  unsigned iv_len;
  unsigned auth_tag_len;
  unsigned max_expansion; 
} cryptoalg_ctx_t;

typedef cryptoalg_ctx_t *cryptoalg_t;

#define cryptoalg_get_key_len(cryptoalg) ((cryptoalg)->key_len)

#define cryptoalg_get_iv_len(cryptoalg) ((cryptoalg)->iv_len)

#define cryptoalg_get_auth_tag_len(cryptoalg) ((cryptoalg)->auth_tag_len)

int
cryptoalg_get_id(cryptoalg_t c);

cryptoalg_t 
cryptoalg_find_by_id(int id);






#endif 


