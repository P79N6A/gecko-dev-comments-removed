






































#ifndef GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__
#define GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__

#ifndef _WIN32

#include <sys/types.h>

#else  

#include <WTypes.h>

typedef unsigned __int8  u_int8_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int64 u_int64_t;

#endif  

typedef struct {
  u_int64_t high;
  u_int64_t low;
} u_int128_t;

typedef u_int64_t breakpad_time_t;

#endif  
