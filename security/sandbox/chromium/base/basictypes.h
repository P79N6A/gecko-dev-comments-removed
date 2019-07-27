









#ifndef BASE_BASICTYPES_H_
#define BASE_BASICTYPES_H_

#include <limits.h>  
#include <stddef.h>  
#include <stdint.h>  

#include "base/macros.h"
#include "base/port.h"  


typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint16_t uint16;
typedef uint32_t uint32;








#if defined(__LP64__) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
typedef long int64;
typedef unsigned long uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif


const uint8  kuint8max  =  0xFF;
const uint16 kuint16max =  0xFFFF;
const uint32 kuint32max =  0xFFFFFFFF;
const uint64 kuint64max =  0xFFFFFFFFFFFFFFFFULL;
const  int8  kint8min   = -0x7F - 1;
const  int8  kint8max   =  0x7F;
const  int16 kint16min  = -0x7FFF - 1;
const  int16 kint16max  =  0x7FFF;
const  int32 kint32min  = -0x7FFFFFFF - 1;
const  int32 kint32max  =  0x7FFFFFFF;
const  int64 kint64min  = -0x7FFFFFFFFFFFFFFFLL - 1;
const  int64 kint64max  =  0x7FFFFFFFFFFFFFFFLL;

#endif  
