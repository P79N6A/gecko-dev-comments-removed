



















#ifndef AVUTIL_BLOWFISH_H
#define AVUTIL_BLOWFISH_H

#include <stdint.h>







#define AV_BF_ROUNDS 16

typedef struct AVBlowfish {
    uint32_t p[AV_BF_ROUNDS + 2];
    uint32_t s[4][256];
} AVBlowfish;








void av_blowfish_init(struct AVBlowfish *ctx, const uint8_t *key, int key_len);









void av_blowfish_crypt_ecb(struct AVBlowfish *ctx, uint32_t *xl, uint32_t *xr,
                           int decrypt);











void av_blowfish_crypt(struct AVBlowfish *ctx, uint8_t *dst, const uint8_t *src,
                       int count, uint8_t *iv, int decrypt);





#endif 
