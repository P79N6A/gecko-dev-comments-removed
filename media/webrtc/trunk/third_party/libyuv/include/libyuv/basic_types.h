









#ifndef INCLUDE_LIBYUV_BASIC_TYPES_H_
#define INCLUDE_LIBYUV_BASIC_TYPES_H_

#include <stddef.h>  

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>  
#endif

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

#define ALIGNP(p, t) \
  (reinterpret_cast<uint8*>(((reinterpret_cast<uintptr_t>(p) + \
  ((t)-1)) & ~((t)-1))))

#endif 
