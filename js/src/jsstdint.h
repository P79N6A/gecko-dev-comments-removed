



















































#ifndef jsstdint_h___
#define jsstdint_h___

#include "jsinttypes.h"




#if ! defined(JS_HAVE_STDINT_H)

typedef JSInt8  int8_t;
typedef JSInt16 int16_t;
typedef JSInt32 int32_t;
typedef JSInt64 int64_t;

typedef JSUint8  uint8_t;
typedef JSUint16 uint16_t;
typedef JSUint32 uint32_t;
typedef JSUint64 uint64_t;





#if !defined(JS_STDDEF_H_HAS_INTPTR_T) && !defined(JS_CRTDEFS_H_HAS_INTPTR_T)
typedef JSIntPtr  intptr_t;
typedef JSUintPtr uintptr_t;
#endif

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

#define INT8_MAX  127
#define INT8_MIN  (-INT8_MAX - 1)
#define INT16_MAX 32767
#define INT16_MIN (-INT16_MAX - 1)
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX - 1)
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX - 1)

#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615ULL






#define INTPTR_MAX  ((intptr_t) (UINTPTR_MAX >> 1))
#define INTPTR_MIN  (intptr_t(uintptr_t(INTPTR_MAX) + uintptr_t(1)))
#define UINTPTR_MAX ((uintptr_t) -1)
#define SIZE_MAX UINTPTR_MAX
#define PTRDIFF_MAX INTPTR_MAX
#define PTRDIFF_MIN INTPTR_MIN

#endif 

#endif 

#endif 
