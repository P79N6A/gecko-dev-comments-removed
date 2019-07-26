












#ifndef WEBRTC_TYPEDEFS_H_
#define WEBRTC_TYPEDEFS_H_



#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || \
    defined(WEBRTC_ANDROID) || defined(WEBRTC_BSD)
#define WEBRTC_POSIX
#endif





#if defined(_M_X64) || defined(__x86_64__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86_64
#define WEBRTC_ARCH_64_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#elif defined(_M_IX86) || defined(__i386__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86
#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#elif defined(__ARMEL__)







#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#elif defined(__powerpc64__)
#define WEBRTC_ARCH_PPC64 1
#define WEBRTC_ARCH_64_BITS 1
#ifdef __LITTLE_ENDIAN__
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#else
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#endif
#elif defined(__ppc__) || defined(__powerpc__)
#define WEBRTC_ARCH_PPC 1
#define WEBRTC_ARCH_32_BITS 1
#ifdef __LITTLE_ENDIAN__
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#else
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#endif
#elif defined(__sparc64__)
#define WEBRTC_ARCH_SPARC 1
#define WEBRTC_ARCH_64_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#elif defined(__sparc__)
#define WEBRTC_ARCH_SPARC 1
#define WEBRTC_ARCH_32_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#elif defined(__mips__)
#define WEBRTC_ARCH_MIPS 1
#if defined(_ABI64) && _MIPS_SIM == _ABI64
#define WEBRTC_ARCH_64_BITS 1
#else
#define WEBRTC_ARCH_32_BITS 1
#endif
#if defined(__MIPSEB__)
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#else
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#endif
#elif defined(__hppa__)
#define WEBRTC_ARCH_HPPA 1
#define WEBRTC_ARCH_32_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#elif defined(__ia64__)
#define WEBRTC_ARCH_IA64 1
#define WEBRTC_ARCH_64_BITS 1
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__s390x__)
#define WEBRTC_ARCH_S390X 1
#define WEBRTC_ARCH_64_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#elif defined(__s390__)
#define WEBRTC_ARCH_S390 1
#define WEBRTC_ARCH_32_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#elif defined(__aarch64__)
#define WEBRTC_ARCH_AARCH64 1
#define WEBRTC_ARCH_64_BITS 1
#if defined(__AARCH64EL__)
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__AARCH64EB__)
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#endif
#elif defined(__alpha__)
#define WEBRTC_ARCH_ALPHA 1
#define WEBRTC_ARCH_64_BITS 1
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__avr32__)
#define WEBRTC_ARCH_AVR32 1
#define WEBRTC_ARCH_32_BITS 1
#define WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_BIG_ENDIAN
#else
#error Please add support for your architecture in typedefs.h
#endif

#if !(defined(WEBRTC_ARCH_LITTLE_ENDIAN) ^ defined(WEBRTC_ARCH_BIG_ENDIAN))
#error Define either WEBRTC_ARCH_LITTLE_ENDIAN or WEBRTC_ARCH_BIG_ENDIAN
#endif

#if defined(__SSE2__) || defined(_MSC_VER)
#define WEBRTC_USE_SSE2
#endif

#if !defined(_MSC_VER)
#include <stdint.h>
#else

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef __int64             int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned __int64    uint64_t;
#endif






#if defined(_MSC_VER)
#define OVERRIDE override
#elif defined(__clang__)




#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700

#define OVERRIDE override
#else
#define OVERRIDE
#endif






#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  

#endif  
