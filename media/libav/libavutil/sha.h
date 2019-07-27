



















#ifndef AVUTIL_SHA_H
#define AVUTIL_SHA_H

#include <stdint.h>

#include "attributes.h"
#include "version.h"







#if FF_API_CONTEXT_SIZE
extern attribute_deprecated const int av_sha_size;
#endif

struct AVSHA;




struct AVSHA *av_sha_alloc(void);








int av_sha_init(struct AVSHA* context, int bits);








void av_sha_update(struct AVSHA* context, const uint8_t* data, unsigned int len);







void av_sha_final(struct AVSHA* context, uint8_t *digest);





#endif 
