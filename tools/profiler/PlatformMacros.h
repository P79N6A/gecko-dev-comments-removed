




#ifndef SPS_PLATFORM_MACROS_H
#define SPS_PLATFORM_MACROS_H







#undef SPS_PLAT_arm_android
#undef SPS_PLAT_amd64_linux
#undef SPS_PLAT_x86_linux
#undef SPS_PLAT_amd64_darwin
#undef SPS_PLAT_x86_darwin
#undef SPS_PLAT_x86_windows
#undef SPS_PLAT_amd64_windows

#undef SPS_ARCH_arm
#undef SPS_ARCH_x86
#undef SPS_ARCH_amd64

#undef SPS_OS_android
#undef SPS_OS_linux
#undef SPS_OS_darwin
#undef SPS_OS_windows

#if defined(__linux__) && defined(__x86_64__)
#  define SPS_PLAT_amd64_linux 1
#  define SPS_ARCH_amd64 1
#  define SPS_OS_linux 1

#elif defined(__ANDROID__) && defined(__arm__)
#  define SPS_PLAT_arm_android 1
#  define SPS_ARCH_arm 1
#  define SPS_OS_android 1

#elif defined(__ANDROID__) && defined(__i386__)
#  define SPS_PLAT_x86_android 1
#  define SPS_ARCH_x86 1
#  define SPS_OS_android 1

#elif defined(__linux__) && defined(__i386__)
#  define SPS_PLAT_x86_linux 1
#  define SPS_ARCH_x86 1
#  define SPS_OS_linux 1

#elif defined(__APPLE__) && defined(__x86_64__)
#  define SPS_PLAT_amd64_darwin 1
#  define SPS_ARCH_amd64 1
#  define SPS_OS_darwin 1

#elif defined(__APPLE__) && defined(__i386__)
#  define SPS_PLAT_x86_darwin 1
#  define SPS_ARCH_x86 1
#  define SPS_OS_darwin 1

#elif defined(_MSC_VER) && defined(_M_IX86)
#  define SPS_PLAT_x86_windows 1
#  define SPS_ARCH_x86 1
#  define SPS_OS_windows 1

#elif defined(_MSC_VER) && defined(_M_X64)
#  define SPS_PLAT_amd64_windows 1
#  define SPS_ARCH_amd64 1
#  define SPS_OS_windows 1

#else
#  error "Unsupported platform"
#endif

#endif 
