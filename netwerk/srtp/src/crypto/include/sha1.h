













































#ifndef SHA1_H
#define SHA1_H

#include "err.h"
#include "datatypes.h"

typedef struct {
  uint32_t H[5];             
  uint32_t M[16];            
  int octets_in_buffer;      
  uint32_t num_bits_in_msg;  
} sha1_ctx_t;








void
sha1(const uint8_t *message,  int octets_in_msg, uint32_t output[5]);












void
sha1_init(sha1_ctx_t *ctx);

void
sha1_update(sha1_ctx_t *ctx, const uint8_t *M, int octets_in_msg);

void
sha1_final(sha1_ctx_t *ctx, uint32_t output[5]);
















void
sha1_core(const uint32_t M[16], uint32_t hash_value[5]);
     
#endif 
