



















#ifndef AVUTIL_AES_H
#define AVUTIL_AES_H

#include <stdint.h>

#include "attributes.h"
#include "version.h"







#if FF_API_CONTEXT_SIZE
extern attribute_deprecated const int av_aes_size;
#endif

struct AVAES;




struct AVAES *av_aes_alloc(void);






int av_aes_init(struct AVAES *a, const uint8_t *key, int key_bits, int decrypt);









void av_aes_crypt(struct AVAES *a, uint8_t *dst, const uint8_t *src, int count, uint8_t *iv, int decrypt);





#endif 
