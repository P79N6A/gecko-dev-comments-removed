



















































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

#endif 

#endif 
