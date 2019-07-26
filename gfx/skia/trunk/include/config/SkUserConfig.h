








#ifndef SkUserConfig_DEFINED
#define SkUserConfig_DEFINED






















































#define SK_ENABLE_INST_COUNT 0







































































#ifdef SK_DEBUG

#endif















#ifdef SK_SAMPLES_FOR_X
        #define SK_R32_SHIFT    16
        #define SK_G32_SHIFT    8
        #define SK_B32_SHIFT    0
        #define SK_A32_SHIFT    24
#endif





























#  if defined(SK_BUILD_FOR_WIN32)
#    define SK_ATOMICS_PLATFORM_H "skia/SkAtomics_win.h"
#  elif defined(SK_BUILD_FOR_ANDROID_FRAMEWORK)
#    define SK_ATOMICS_PLATFORM_H "skia/SkAtomics_android.h"
#  else
#    define SK_ATOMICS_PLATFORM_H "skia/SkAtomics_sync.h"
#  endif

#  if defined(SK_BUILD_FOR_WIN32)
#    define SK_MUTEX_PLATFORM_H "skia/SkMutex_win.h"
#  else
#    define SK_MUTEX_PLATFORM_H "skia/SkMutex_pthread.h"
#  endif
#endif

#define SK_ALLOW_STATIC_GLOBAL_INITIALIZERS 0

#define SK_SUPPORT_LEGACY_LAYERRASTERIZER_API 1
#define SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG 1
#define SK_SUPPORT_LEGACY_GETTOTALCLIP 1
