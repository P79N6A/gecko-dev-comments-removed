



#ifndef _SDP_OS_DEFS_H_
#define _SDP_OS_DEFS_H_


#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "phone_debug.h"


#define SDP_ERROR     buginf
#define SDP_WARN      buginf
#define SDP_PRINT     buginf
#define SDP_MALLOC(x) cpr_calloc(1, (x))
#define SDP_FREE      cpr_free

typedef uint8_t    tinybool;
typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint16_t   uint16;
typedef uint32_t   u32;
typedef uint32_t   uint32;
typedef int32_t    int32;
typedef int16_t    int16;
typedef unsigned short ushort;
typedef unsigned long  ulong;
#define inline 


#endif 
