











































#ifndef XFM_H
#define XFM_H

#include "crypto_kernel.h"
#include "err.h"




































typedef err_status_t (*xfm_func_t) 
     (void *key,            
      void *clear,          
      unsigned clear_len,   
      void *iv,             
      void *protect,         
      unsigned *protected_len, 
      void *auth_tag        
      );

typedef 
err_status_t (*xfm_inv_t)
     (void *key,            
      void *clear,          
      unsigned clear_len,   
      void *iv,             
      void *opaque,         
      unsigned *opaque_len, 


      void *auth_tag        
      );

typedef struct xfm_ctx_t {
  xfm_func_t func;
  xfm_inv_t  inv;
  unsigned key_len;
  unsigned iv_len;
  unsigned auth_tag_len;
} xfm_ctx_t;

typedef xfm_ctx_t *xfm_t;

#define xfm_get_key_len(xfm) ((xfm)->key_len)

#define xfm_get_iv_len(xfm) ((xfm)->iv_len)

#define xfm_get_auth_tag_len(xfm) ((xfm)->auth_tag_len)



  
typedef err_status_t (*cryptoalg_func_t) 
     (void *key,            
      void *clear,          
      unsigned clear_len,   
      void *iv,             
      void *opaque,         
      unsigned *opaque_len
      );

typedef 
err_status_t (*cryptoalg_inv_t)
     (void *key,            
      void *clear,          
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







#endif 


