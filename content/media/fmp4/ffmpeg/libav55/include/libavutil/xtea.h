



















#ifndef AVUTIL_XTEA_H
#define AVUTIL_XTEA_H

#include <stdint.h>







typedef struct AVXTEA {
    uint32_t key[16];
} AVXTEA;







void av_xtea_init(struct AVXTEA *ctx, const uint8_t key[16]);











void av_xtea_crypt(struct AVXTEA *ctx, uint8_t *dst, const uint8_t *src,
                   int count, uint8_t *iv, int decrypt);





#endif 
