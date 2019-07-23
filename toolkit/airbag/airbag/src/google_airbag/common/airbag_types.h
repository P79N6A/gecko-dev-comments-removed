






































#ifndef GOOGLE_AIRBAG_COMMON_AIRBAG_TYPES_H__
#define GOOGLE_AIRBAG_COMMON_AIRBAG_TYPES_H__

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
  u_int64_t half[2];
} u_int128_t;

typedef u_int64_t airbag_time_t;

#endif  
