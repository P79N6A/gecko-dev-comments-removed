



































#ifndef jsinttypes_h___
#define jsinttypes_h___

#include "js-config.h"





























#if defined(JS_HAVE_STDINT_H) || \
    defined(JS_SYS_TYPES_H_DEFINES_EXACT_SIZE_TYPES)

#if defined(JS_HAVE_STDINT_H)
#include <stdint.h>
#else
#include <sys/types.h>
#endif

typedef int8_t   JSInt8;
typedef int16_t  JSInt16;
typedef int32_t  JSInt32;
typedef int64_t  JSInt64;
typedef intptr_t JSIntPtr;

typedef uint8_t   JSUint8;
typedef uint16_t  JSUint16;
typedef uint32_t  JSUint32;
typedef uint64_t  JSUint64;
typedef uintptr_t JSUintPtr;

#else

#if defined(JS_HAVE___INTN)

typedef __int8  JSInt8;
typedef __int16 JSInt16;
typedef __int32 JSInt32;
typedef __int64 JSInt64;

typedef unsigned __int8 JSUint8;
typedef unsigned __int16 JSUint16;
typedef unsigned __int32 JSUint32;
typedef unsigned __int64 JSUint64;

#elif defined(JS_INT8_TYPE)

typedef signed JS_INT8_TYPE   JSInt8;
typedef signed JS_INT16_TYPE  JSInt16;
typedef signed JS_INT32_TYPE  JSInt32;
typedef signed JS_INT64_TYPE  JSInt64;

typedef unsigned JS_INT8_TYPE   JSUint8;
typedef unsigned JS_INT16_TYPE  JSUint16;
typedef unsigned JS_INT32_TYPE  JSUint32;
typedef unsigned JS_INT64_TYPE  JSUint64;

#else
#error "couldn't find exact-width integer types"
#endif


#if defined(JS_STDDEF_H_HAS_INTPTR_T)
#include <stddef.h>
typedef intptr_t JSIntPtr;
typedef uintptr_t JSUintPtr;


#elif defined(JS_INTPTR_TYPE)
typedef signed   JS_INTPTR_TYPE JSIntPtr;
typedef unsigned JS_INTPTR_TYPE JSUintPtr;

#else
#error "couldn't find pointer-sized integer types"
#endif

#endif 

#endif 
