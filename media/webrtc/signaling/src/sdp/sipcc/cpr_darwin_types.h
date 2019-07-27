



#ifndef _CPR_DARWIN_TYPES_H_
#define _CPR_DARWIN_TYPES_H_

#include <sys/types.h>
#include <sys/param.h>
#include <stddef.h>
#include "inttypes.h"












typedef uint8_t boolean;















#ifndef MIN
#ifdef __GNUC__
#define MIN(a,b)  ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#else
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#ifndef MAX
#ifdef __GNUC__
#define MAX(a,b)  ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#else
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif





#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
