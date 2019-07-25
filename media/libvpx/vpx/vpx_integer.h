










#ifndef VPX_INTEGER_H
#define VPX_INTEGER_H


#include <stddef.h>

#if !defined(VPX_DONT_DEFINE_STDINT_TYPES)

#if (defined(_MSC_VER) && (_MSC_VER < 1600)) || defined(VPX_EMULATE_INTTYPES)
typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed int   int32_t;

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#if (defined(_MSC_VER) && (_MSC_VER < 1600))
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#endif

#ifndef _UINTPTR_T_DEFINED
typedef size_t uintptr_t;
#endif

#else



#if defined(__cplusplus) && !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#include <stdint.h>

#endif

#endif


#if defined(_MSC_VER)
#define PRId64 "I64d"
#else
#include <inttypes.h>
#endif

#endif
