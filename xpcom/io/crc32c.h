#ifndef crc32c_h
#define crc32c_h

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif






uint32_t
ComputeCrc32c(uint32_t aCrc, const void *aBuf, size_t aSize);

#ifdef __cplusplus
} 
#endif

#endif
