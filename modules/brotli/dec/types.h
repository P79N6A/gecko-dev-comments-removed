
















#ifndef BROTLI_DEC_TYPES_H_
#define BROTLI_DEC_TYPES_H_

#include <stddef.h>  

#ifndef _MSC_VER
#include <inttypes.h>
#ifdef __STRICT_ANSI__
#define BROTLI_INLINE
#else  
#define BROTLI_INLINE inline
#endif
#else
typedef signed   char int8_t;
typedef unsigned char uint8_t;
typedef signed   short int16_t;
typedef unsigned short uint16_t;
typedef signed   int int32_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;
typedef long long int int64_t;
#define BROTLI_INLINE __forceinline
#endif  

#endif  
