












#ifndef WEBRTC_TYPEDEFS_H_
#define WEBRTC_TYPEDEFS_H_



#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || \
    defined(WEBRTC_ANDROID)
#define WEBRTC_POSIX
#endif






#if defined(_M_X64) || defined(__x86_64__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86_64
#define WEBRTC_ARCH_64_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(_M_IX86) || defined(__i386__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86
#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__ARMEL__)







#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__MIPSEL__)
#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#else
#error Please add support for your architecture in typedefs.h
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
#else
#define OVERRIDE
#endif

#endif  
