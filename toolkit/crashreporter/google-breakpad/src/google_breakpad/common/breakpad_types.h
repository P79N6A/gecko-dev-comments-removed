






































#ifndef GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__
#define GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__

#ifndef _WIN32

#include <sys/types.h>

#if defined(__SUNPRO_CC) || (defined(__GNUC__) && defined(__sun__))
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#endif

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
