



































#include "opus_types.h"

#if !defined(_entcode_H)
# define _entcode_H (1)
# include <limits.h>
# include <stddef.h>
# include "ecintrin.h"



typedef opus_uint32           ec_window;
typedef struct ec_ctx         ec_ctx;
typedef struct ec_ctx         ec_enc;
typedef struct ec_ctx         ec_dec;

# define EC_WINDOW_SIZE ((int)sizeof(ec_window)*CHAR_BIT)


# define EC_UINT_BITS   (8)



# define BITRES 3




struct ec_ctx{
   
   unsigned char *buf;
   
   opus_uint32    storage;
   
   opus_uint32    end_offs;
   
   ec_window      end_window;
   
   int            nend_bits;
   

   int            nbits_total;
   
   opus_uint32    offs;
   
   opus_uint32    rng;
   


   opus_uint32    val;
   

   opus_uint32    ext;
   
   int            rem;
   
   int            error;
};

static inline opus_uint32 ec_range_bytes(ec_ctx *_this){
  return _this->offs;
}

static inline unsigned char *ec_get_buffer(ec_ctx *_this){
  return _this->buf;
}

static inline int ec_get_error(ec_ctx *_this){
  return _this->error;
}







static inline int ec_tell(ec_ctx *_this){
  return _this->nbits_total-EC_ILOG(_this->rng);
}







opus_uint32 ec_tell_frac(ec_ctx *_this);

#endif
