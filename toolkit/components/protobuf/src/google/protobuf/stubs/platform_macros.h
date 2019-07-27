





























#ifndef GOOGLE_PROTOBUF_PLATFORM_MACROS_H_
#define GOOGLE_PROTOBUF_PLATFORM_MACROS_H_

#include <google/protobuf/stubs/common.h>

#define GOOGLE_PROTOBUF_PLATFORM_ERROR \
#error "Host platform was not detected as supported by protobuf"





#if defined(_M_X64) || defined(__x86_64__)
#define GOOGLE_PROTOBUF_ARCH_X64 1
#define GOOGLE_PROTOBUF_ARCH_64_BIT 1
#elif defined(_M_IX86) || defined(__i386__)
#define GOOGLE_PROTOBUF_ARCH_IA32 1
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#elif defined(__QNX__)
#define GOOGLE_PROTOBUF_ARCH_ARM_QNX 1
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#elif defined(__ARMEL__)
#define GOOGLE_PROTOBUF_ARCH_ARM 1
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#elif defined(__aarch64__)
#define GOOGLE_PROTOBUF_ARCH_AARCH64 1
#define GOOGLE_PROTOBUF_ARCH_64_BIT 1
#elif defined(__MIPSEL__)
#if defined(__LP64__)
#define GOOGLE_PROTOBUF_ARCH_MIPS64 1
#define GOOGLE_PROTOBUF_ARCH_64_BIT 1
#else
#define GOOGLE_PROTOBUF_ARCH_MIPS 1
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#endif
#elif defined(__pnacl__)
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#elif defined(sparc)
#define GOOGLE_PROTOBUF_ARCH_SPARC 1
#ifdef SOLARIS_64BIT_ENABLED
#define GOOGLE_PROTOBUF_ARCH_64_BIT 1
#else
#define GOOGLE_PROTOBUF_ARCH_32_BIT 1
#endif
#elif defined(__GNUC__)
# if (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)) || (__GNUC__ > 4))

# elif defined(__clang__)
#  if !__has_extension(c_atomic)
GOOGLE_PROTOBUF_PLATFORM_ERROR
#  endif

# endif
# if __LP64__
#  define GOOGLE_PROTOBUF_ARCH_64_BIT 1
# else
#  define GOOGLE_PROTOBUF_ARCH_32_BIT 1
# endif
#else
GOOGLE_PROTOBUF_PLATFORM_ERROR
#endif

#if defined(__APPLE__)
#define GOOGLE_PROTOBUF_OS_APPLE
#elif defined(__native_client__)
#define GOOGLE_PROTOBUF_OS_NACL
#elif defined(sun)
#define GOOGLE_PROTOBUF_OS_SOLARIS
#endif

#undef GOOGLE_PROTOBUF_PLATFORM_ERROR

#endif  
