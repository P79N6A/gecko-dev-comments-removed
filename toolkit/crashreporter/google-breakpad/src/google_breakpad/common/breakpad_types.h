






































#ifndef GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__
#define GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__

#ifndef _WIN32

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  
#include <inttypes.h>

#else  

#if _MSC_VER >= 1600
#include <stdint.h>
#elif defined(BREAKPAD_CUSTOM_STDINT_H)




#include BREAKPAD_CUSTOM_STDINT_H
#else
#include <wtypes.h>

typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#endif  

typedef struct {
  uint64_t high;
  uint64_t low;
} uint128_struct;

typedef uint64_t breakpad_time_t;




#ifndef PRIx64
#define PRIx64 "llx"
#endif  

#endif  
