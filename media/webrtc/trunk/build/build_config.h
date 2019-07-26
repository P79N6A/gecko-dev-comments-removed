












#ifndef BUILD_BUILD_CONFIG_H_
#define BUILD_BUILD_CONFIG_H_

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif


#if defined(__APPLE__)
#define OS_MACOSX 1
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define OS_IOS 1
#endif  
#elif defined(ANDROID)
#define OS_ANDROID 1
#elif defined(__native_client__)
#define OS_NACL 1
#elif defined(__linux__)
#define OS_LINUX 1

#if !defined(TOOLKIT_VIEWS)
#define TOOLKIT_GTK
#endif
#elif defined(_WIN32)
#define OS_WIN 1
#define TOOLKIT_VIEWS 1
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#define TOOLKIT_GTK
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#define TOOLKIT_GTK
#elif defined(__sun)
#define OS_SOLARIS 1
#define TOOLKIT_GTK
#else
#error Please add support for your platform in build/build_config.h
#endif

#if defined(USE_OPENSSL) && defined(USE_NSS)
#error Cannot use both OpenSSL and NSS
#endif



#if defined(OS_FREEBSD) || defined(OS_OPENBSD)
#define OS_BSD 1
#endif



#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_FREEBSD) ||     \
    defined(OS_OPENBSD) || defined(OS_SOLARIS) || defined(OS_ANDROID) ||  \
    defined(OS_NACL)
#define OS_POSIX 1
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(OS_NACL)
#define USE_X11 1  // Use X for graphics.
#endif


#if (defined(OS_WIN) || defined(OS_LINUX)) && !defined(NO_TCMALLOC)
#define USE_TCMALLOC 1
#endif


#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#error Please add support for your compiler in build/build_config.h
#endif





#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARMEL 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__pnacl__)
#define ARCH_CPU_32_BITS 1
#elif defined(__MIPSEL__)
#define ARCH_CPU_MIPS_FAMILY 1
#define ARCH_CPU_MIPSEL 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__powerpc64__)
#define ARCH_CPU_PPC_FAMILY 1
#define ARCH_CPU_PPC64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_BIG_ENDIAN 1
#elif defined(__ppc__) || defined(__powerpc__)
#define ARCH_CPU_PPC_FAMILY 1
#define ARCH_CPU_PPC 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_BIG_ENDIAN 1
#elif defined(__sparc64__)
#define ARCH_CPU_SPARC_FAMILY 1
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_64_BITS 1
#elif defined(__sparc__)
#define ARCH_CPU_SPARC_FAMILY 1
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_32_BITS 1
#elif defined(__mips__)
#define ARCH_CPU_MIPS_FAMILY 1
#define ARCH_CPU_MIPS 1
#define ARCH_CPU_32_BITS 1
#elif defined(__hppa__)
#define ARCH_CPU_HPPA 1
#define ARCH_CPU_32_BITS 1
#elif defined(__ia64__)
#define ARCH_CPU_IA64 1
#define ARCH_CPU_64_BITS 1
#elif defined(__s390x__)
#define ARCH_CPU_S390X 1
#define ARCH_CPU_64_BITS 1
#elif defined(__s390__)
#define ARCH_CPU_S390 1
#define ARCH_CPU_32_BITS 1
#elif defined(__alpha__)
#define ARCH_CPU_ALPHA 1
#define ARCH_CPU_64_BITS 1
#else
#error Please add support for your architecture in build/build_config.h
#endif


#if defined(OS_WIN)
#define WCHAR_T_IS_UTF16
#elif defined(OS_POSIX) && defined(COMPILER_GCC) && \
    defined(__WCHAR_MAX__) && \
    (__WCHAR_MAX__ == 0x7fffffff || __WCHAR_MAX__ == 0xffffffff)
#define WCHAR_T_IS_UTF32
#elif defined(OS_POSIX) && defined(COMPILER_GCC) && \
    defined(__WCHAR_MAX__) && \
    (__WCHAR_MAX__ == 0x7fff || __WCHAR_MAX__ == 0xffff)




#define WCHAR_T_IS_UTF16
#else
#error Please add support for your compiler in build/build_config.h
#endif

#if defined(__ARMEL__) && !defined(OS_IOS)
#define WCHAR_T_IS_UNSIGNED 1
#elif defined(__MIPSEL__)
#define WCHAR_T_IS_UNSIGNED 0
#endif

#if defined(OS_ANDROID)


#define STD_STRING_ITERATOR_IS_CHAR_POINTER


#define BASE_STRING16_ITERATOR_IS_CHAR16_POINTER
#endif

#endif  
