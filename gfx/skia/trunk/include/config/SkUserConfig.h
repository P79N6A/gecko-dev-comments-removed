








#ifndef SkUserConfig_DEFINED
#define SkUserConfig_DEFINED






















































#define SK_ENABLE_INST_COUNT 0







































































#ifdef SK_DEBUG

#endif








































#if defined(_MSC_VER)
#  define SK_ATOMICS_PLATFORM_H "skia/SkAtomics_win.h"
#else
#  define SK_ATOMICS_PLATFORM_H "skia/SkAtomics_sync.h"
#endif

#if defined(_WIN32)
#  define SK_MUTEX_PLATFORM_H   "skia/SkMutex_win.h"
#else
#  define SK_MUTEX_PLATFORM_H   "skia/SkMutex_pthread.h"
#endif

#if defined(SK_CPU_ARM32) || defined(SK_CPU_ARM64)
#  define SK_BARRIERS_PLATFORM_H "skia/SkBarriers_arm.h"
#else
#  define SK_BARRIERS_PLATFORM_H "skia/SkBarriers_x86.h"
#endif


#define SK_A32_SHIFT 24
#define SK_R32_SHIFT 16
#define SK_G32_SHIFT 8
#define SK_B32_SHIFT 0

#define SK_ALLOW_STATIC_GLOBAL_INITIALIZERS 0

#define SK_SUPPORT_LEGACY_GETDEVICE
#define SK_IGNORE_ETC1_SUPPORT

#define SK_RASTERIZE_EVEN_ROUNDING

#endif
