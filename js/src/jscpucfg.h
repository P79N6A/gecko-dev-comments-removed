






































#ifndef js_cpucfg___
#define js_cpucfg___

#define JS_HAVE_LONG_LONG

#if defined(_WIN64)

#if defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   8L
#define JS_BITS_PER_WORD_LOG2   6
#define JS_ALIGN_OF_POINTER 8L
#else  
#error "CPU type is unknown"
#endif 

#elif defined(_WIN32) || defined(XP_OS2)

#ifdef __WATCOMC__
#define HAVE_VA_LIST_AS_ARRAY 1
#endif

#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   4L
#define JS_BITS_PER_WORD_LOG2   5
#define JS_ALIGN_OF_POINTER 4L

#elif defined(__APPLE__)
#if __LITTLE_ENDIAN__
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#elif __BIG_ENDIAN__
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1
#endif

#elif defined(JS_HAVE_ENDIAN_H)
#include <endian.h>

#if defined(__BYTE_ORDER)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1
#endif
#else 
#error "endian.h does not define __BYTE_ORDER. Cannot determine endianness."
#endif

#else 
#error "Cannot determine endianness of your platform. Please add support to jscpucfg.h."
#endif

#ifndef JS_STACK_GROWTH_DIRECTION
#define JS_STACK_GROWTH_DIRECTION (-1)
#endif

#endif 
