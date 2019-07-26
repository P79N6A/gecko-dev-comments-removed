









#ifndef INCLUDE_LIBYUV_BASIC_TYPES_H_  
#define INCLUDE_LIBYUV_BASIC_TYPES_H_

#include <stddef.h>  

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>  
#endif

#include "mozilla/StandardInteger.h"
typedef uint64_t uint64;
typedef int64_t  int64;
#if defined(_MSC_VER)

typedef long int32;
typedef unsigned long uint32;
#else
typedef uint32_t uint32;
typedef int32_t  int32;
#endif
typedef uint16_t uint16;
typedef int16_t  int16;
typedef uint8_t  uint8;
typedef int8_t   int8;
#define INT_TYPES_DEFINED 1

#ifndef INT_TYPES_DEFINED
#define INT_TYPES_DEFINED
#ifdef COMPILER_MSVC
typedef unsigned __int64 uint64;
typedef __int64 int64;
#ifndef INT64_C
#define INT64_C(x) x ## I64
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UI64
#endif
#define INT64_F "I64"
#else  
#ifdef __LP64__
typedef unsigned long uint64;  
typedef long int64;  
#ifndef INT64_C
#define INT64_C(x) x ## L
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UL
#endif
#define INT64_F "l"
#else  
typedef unsigned long long uint64;  
typedef long long int64;  
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "ll"
#endif  
#endif  
typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;  
typedef short int16;  
typedef unsigned char uint8;
typedef signed char int8;
#endif  


#if defined(__x86_64__) || defined(_M_X64) || \
    defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif

#if defined(__arm__) || defined(_M_ARM)
#define CPU_ARM 1
#endif

#ifndef ALIGNP
#define ALIGNP(p, t) \
    (reinterpret_cast<uint8*>(((reinterpret_cast<uintptr_t>(p) + \
    ((t) - 1)) & ~((t) - 1))))
#endif

#if !defined(LIBYUV_API)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(LIBYUV_BUILDING_SHARED_LIBRARY)
#define LIBYUV_API __declspec(dllexport)
#elif defined(LIBYUV_USING_SHARED_LIBRARY)
#define LIBYUV_API __declspec(dllimport)
#else
#define LIBYUV_API
#endif  
#elif defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__APPLE__) && \
    (defined(LIBYUV_BUILDING_SHARED_LIBRARY) || \
    defined(LIBYUV_USING_SHARED_LIBRARY))
#define LIBYUV_API __attribute__ ((visibility ("default")))
#else
#define LIBYUV_API
#endif  
#endif  

#endif  
