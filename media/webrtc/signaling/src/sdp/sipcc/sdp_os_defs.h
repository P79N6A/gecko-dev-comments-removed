



#ifndef _SDP_OS_DEFS_H_
#define _SDP_OS_DEFS_H_

#include <stdlib.h>

#include "cpr_types.h"
#include "cpr_string.h"


#define SDP_PRINT(format, ...) CSFLogError("sdp" , format , ## __VA_ARGS__ )


#define SDP_MALLOC(x) calloc(1, (x))
#define SDP_FREE free

typedef uint8_t    tinybool;
typedef unsigned short ushort;
typedef unsigned long  ulong;
#ifndef __GNUC_STDC_INLINE__
#define inline
#endif

#endif 
