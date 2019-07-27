









#ifndef WEBRTC_BASE_BASICTYPES_H_
#define WEBRTC_BASE_BASICTYPES_H_

#include <stddef.h>  

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>  
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"  
#endif

#include "webrtc/base/constructormagic.h"

#if !defined(INT_TYPES_DEFINED)
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


#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
typedef uint64_t uint64;
typedef int64_t int64;
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "l"
#elif defined(__LP64__)
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
#if defined(CPU_X86) && defined(CPU_ARM)
#error CPU_X86 and CPU_ARM both defined.
#endif
#if !defined(ARCH_CPU_BIG_ENDIAN) && !defined(ARCH_CPU_LITTLE_ENDIAN)

#if CPU_X86 || CPU_ARM ||  \
  (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ARCH_CPU_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ARCH_CPU_BIG_ENDIAN
#else
#error ARCH_CPU_BIG_ENDIAN or ARCH_CPU_LITTLE_ENDIAN should be defined.
#endif
#endif
#if defined(ARCH_CPU_BIG_ENDIAN) && defined(ARCH_CPU_LITTLE_ENDIAN)
#error ARCH_CPU_BIG_ENDIAN and ARCH_CPU_LITTLE_ENDIAN both defined.
#endif

#if defined(WEBRTC_WIN)
typedef int socklen_t;
#endif


#ifdef __cplusplus
namespace rtc {
  template<class T> inline T _min(T a, T b) { return (a > b) ? b : a; }
  template<class T> inline T _max(T a, T b) { return (a < b) ? b : a; }

  
  
  const int kForever = -1;
}

#define ALIGNP(p, t) \
    (reinterpret_cast<uint8*>(((reinterpret_cast<uintptr_t>(p) + \
    ((t) - 1)) & ~((t) - 1))))
#define RTC_IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))



#define LIBJINGLE_DEFINE_STATIC_LOCAL(type, name, arguments) \
  static type& name = *new type arguments

#endif  
#endif  
