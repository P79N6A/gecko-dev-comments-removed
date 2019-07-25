












#ifndef BUILD_BUILD_CONFIG_H_
#define BUILD_BUILD_CONFIG_H_


#if defined(__APPLE__)
#define OS_MACOSX 1
#elif defined(__linux__) || defined(ANDROID)
#define OS_LINUX 1
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#elif defined(_WIN32)
#define OS_WIN 1
#else
#error Please add support for your platform in build/build_config.h
#endif



#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_OPENBSD)
#define OS_POSIX 1
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
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARMEL 1
#define ARCH_CPU_32_BITS 1
#define WCHAR_T_IS_UNSIGNED 1
#elif defined(__powerpc64__)
#define ARCH_CPU_PPC64 1
#define ARCH_CPU_64_BITS 1
#elif defined(__ppc__) || defined(__powerpc__)
#define ARCH_CPU_PPC 1
#define ARCH_CPU_32_BITS 1
#elif defined(__sparc64__)
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_64_BITS 1
#elif defined(__sparc__)
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_32_BITS 1
#elif defined(__mips__)
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
#else
#error Please add support for your architecture in build/build_config.h
#endif


#if defined(OS_WIN)
#define WCHAR_T_IS_UTF16
#else
#define WCHAR_T_IS_UTF32
#endif

#endif  
