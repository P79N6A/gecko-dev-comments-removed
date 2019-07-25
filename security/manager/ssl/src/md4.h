




#ifndef md4_h__
#define md4_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "mozilla/StandardInteger.h"


















void md4sum(const uint8_t *input, uint32_t inputLen, uint8_t *result);

#ifdef __cplusplus
}
#endif

#endif
