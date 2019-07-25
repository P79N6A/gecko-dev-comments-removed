





















































#ifndef PROTYPES_H
#define PROTYPES_H

#include "mozilla/StdInt.h"

#include "js-config.h"

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;







#if defined(AIX) && defined(HAVE_SYS_INTTYPES_H)
#include <sys/inttypes.h>
#else
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
#endif 

typedef uint8_t JSUint8;
typedef uint16_t JSUint16;
typedef uint32_t JSUint32;
typedef uint64_t JSUint64;

typedef int8_t JSInt8;
typedef int16_t JSInt16;
typedef int32_t JSInt32;
typedef int64_t JSInt64;

#endif 
