









































#ifndef jsstdint_h___
#define jsstdint_h___

#include "js-config.h"


#if defined(JS_HAVE_STDINT_H)
#include <stdint.h>



#elif defined(JS_INT8_TYPE)

typedef signed   JS_INT8_TYPE   int8_t;
typedef signed   JS_INT16_TYPE  int16_t;
typedef signed   JS_INT32_TYPE  int32_t;
typedef signed   JS_INT64_TYPE  int64_t;
typedef signed   JS_INTPTR_TYPE intptr_t;

typedef unsigned JS_INT8_TYPE   uint8_t;
typedef unsigned JS_INT16_TYPE  uint16_t;
typedef unsigned JS_INT32_TYPE  uint32_t;
typedef unsigned JS_INT64_TYPE  uint64_t;
typedef unsigned JS_INTPTR_TYPE uintptr_t;

#else


#if defined(JS_HAVE___INTN)

typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#else
#error "couldn't find exact-width integer types"
#endif


#if defined(JS_STDDEF_H_HAS_INTPTR_T)
#include <stddef.h>


#elif defined(JS_CRTDEFS_H_HAS_INTPTR_T)
#include <crtdefs.h>

#else
#error "couldn't find definitions for intptr_t, uintptr_t"
#endif

#endif 

#endif 
