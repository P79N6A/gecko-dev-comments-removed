






#ifndef SkPostConfig_DEFINED
#define SkPostConfig_DEFINED

#if defined(SK_BUILD_FOR_WIN32) || defined(SK_BUILD_FOR_WINCE)
#  define SK_BUILD_FOR_WIN
#endif

#if defined(SK_DEBUG) && defined(SK_RELEASE)
#  error "cannot define both SK_DEBUG and SK_RELEASE"
#elif !defined(SK_DEBUG) && !defined(SK_RELEASE)
#  error "must define either SK_DEBUG or SK_RELEASE"
#endif

#if defined(SK_SUPPORT_UNITTEST) && !defined(SK_DEBUG)
#  error "can't have unittests without debug"
#endif






#if defined(SK_MSCALAR_IS_DOUBLE) && defined(SK_MSCALAR_IS_FLOAT)
#  error "cannot define both SK_MSCALAR_IS_DOUBLE and SK_MSCALAR_IS_FLOAT"
#elif !defined(SK_MSCALAR_IS_DOUBLE) && !defined(SK_MSCALAR_IS_FLOAT)
#  define SK_MSCALAR_IS_DOUBLE
#endif

#if defined(SK_CPU_LENDIAN) && defined(SK_CPU_BENDIAN)
#  error "cannot define both SK_CPU_LENDIAN and SK_CPU_BENDIAN"
#elif !defined(SK_CPU_LENDIAN) && !defined(SK_CPU_BENDIAN)
#  error "must define either SK_CPU_LENDIAN or SK_CPU_BENDIAN"
#endif




#ifdef SK_A32_SHIFT
#  if !defined(SK_R32_SHIFT) || !defined(SK_G32_SHIFT) || !defined(SK_B32_SHIFT)
#    error "all or none of the 32bit SHIFT amounts must be defined"
#  endif
#else
#  if defined(SK_R32_SHIFT) || defined(SK_G32_SHIFT) || defined(SK_B32_SHIFT)
#    error "all or none of the 32bit SHIFT amounts must be defined"
#  endif
#endif

#if !defined(SK_HAS_COMPILER_FEATURE)
#  if defined(__has_feature)
#    define SK_HAS_COMPILER_FEATURE(x) __has_feature(x)
#  else
#    define SK_HAS_COMPILER_FEATURE(x) 0
#  endif
#endif

#if !defined(SK_ATTRIBUTE)
#  if defined(__clang__) || defined(__GNUC__)
#    define SK_ATTRIBUTE(attr) __attribute__((attr))
#  else
#    define SK_ATTRIBUTE(attr)
#  endif
#endif

#if !defined(SK_SUPPORT_GPU)
#  define SK_SUPPORT_GPU 1
#endif








#if !defined(SkNO_RETURN_HINT)
#  if SK_HAS_COMPILER_FEATURE(attribute_analyzer_noreturn)
     static inline void SkNO_RETURN_HINT() __attribute__((analyzer_noreturn));
     static inline void SkNO_RETURN_HINT() {}
#  else
#    define SkNO_RETURN_HINT() do {} while (false)
#  endif
#endif

#if defined(SK_ZLIB_INCLUDE) && defined(SK_SYSTEM_ZLIB)
#  error "cannot define both SK_ZLIB_INCLUDE and SK_SYSTEM_ZLIB"
#elif defined(SK_ZLIB_INCLUDE) || defined(SK_SYSTEM_ZLIB)
#  define SK_HAS_ZLIB
#endif



#ifndef SkNEW
#  define SkNEW(type_name)                           (new type_name)
#  define SkNEW_ARGS(type_name, args)                (new type_name args)
#  define SkNEW_ARRAY(type_name, count)              (new type_name[(count)])
#  define SkNEW_PLACEMENT(buf, type_name)            (new (buf) type_name)
#  define SkNEW_PLACEMENT_ARGS(buf, type_name, args) (new (buf) type_name args)
#  define SkDELETE(obj)                              (delete (obj))
#  define SkDELETE_ARRAY(array)                      (delete[] (array))
#endif

#ifndef SK_CRASH
#  ifdef SK_BUILD_FOR_WIN
#    define SK_CRASH() __debugbreak()
#  else
#    if 1   
#      define SK_CRASH() do { SkNO_RETURN_HINT(); *(int *)(uintptr_t)0xbbadbeef = 0; } while (false)
#    else
#      define SK_CRASH() do { SkNO_RETURN_HINT(); } while (true)
#    endif
#  endif
#endif








#ifndef SK_ENABLE_INST_COUNT
#  ifdef SK_DEBUG


#    define SK_ENABLE_INST_COUNT !defined(SKIA_DLL)
#  else
#    define SK_ENABLE_INST_COUNT 0
#  endif
#endif



#ifdef SK_BUILD_FOR_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#    define NOMINMAX_WAS_LOCALLY_DEFINED
#  endif
#
#  include <windows.h>
#
#  ifdef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_LEAN_AND_MEAN
#  endif
#  ifdef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX
#  endif
#
#  ifndef SK_A32_SHIFT
#    define SK_A32_SHIFT 24
#    define SK_R32_SHIFT 16
#    define SK_G32_SHIFT 8
#    define SK_B32_SHIFT 0
#  endif
#
#endif

#ifndef SK_ALWAYSBREAK
#  ifdef SK_DEBUG
#    define SK_ALWAYSBREAK(cond) do { \
              if (cond) break; \
              SkNO_RETURN_HINT(); \
              SkDebugf("%s:%d: failed assertion \"%s\"\n", __FILE__, __LINE__, #cond); \
              SK_CRASH(); \
        } while (false)
#  else
#    define SK_ALWAYSBREAK(cond) do { if (cond) break; SK_CRASH(); } while (false)
#  endif
#endif






#ifndef SK_A32_SHIFT
#  ifdef SK_CPU_BENDIAN
#    define SK_R32_SHIFT    24
#    define SK_G32_SHIFT    16
#    define SK_B32_SHIFT    8
#    define SK_A32_SHIFT    0
#  else
#    define SK_R32_SHIFT    0
#    define SK_G32_SHIFT    8
#    define SK_B32_SHIFT    16
#    define SK_A32_SHIFT    24
#  endif
#endif






#define SK_COLOR_MATCHES_PMCOLOR_BYTE_ORDER \
    (SK_A32_SHIFT == 24 && SK_R32_SHIFT == 16 && SK_G32_SHIFT == 8 && SK_B32_SHIFT == 0)









#ifdef SK_CPU_BENDIAN
#  define SK_PMCOLOR_BYTE_ORDER(C0, C1, C2, C3)     \
        (SK_ ## C3 ## 32_SHIFT == 0  &&             \
         SK_ ## C2 ## 32_SHIFT == 8  &&             \
         SK_ ## C1 ## 32_SHIFT == 16 &&             \
         SK_ ## C0 ## 32_SHIFT == 24)
#else
#  define SK_PMCOLOR_BYTE_ORDER(C0, C1, C2, C3)     \
        (SK_ ## C0 ## 32_SHIFT == 0  &&             \
         SK_ ## C1 ## 32_SHIFT == 8  &&             \
         SK_ ## C2 ## 32_SHIFT == 16 &&             \
         SK_ ## C3 ## 32_SHIFT == 24)
#endif







#if defined(SK_BUILD_FOR_WIN32) || defined(SK_BUILD_FOR_MAC)
#  ifndef SkLONGLONG
#    define SkLONGLONG int64_t
#  endif
#endif


#ifndef SK_BUILD_FOR_WINCE
#  include <string.h>
#  include <stdlib.h>
#else
#  define _CMNINTRIN_DECLARE_ONLY
#  include "cmnintrin.h"
#endif

#if defined SK_DEBUG && defined SK_BUILD_FOR_WIN32
#  ifdef free
#    undef free
#  endif
#  include <crtdbg.h>
#  undef free
#
#  ifdef SK_DEBUGx
#    if defined(SK_SIMULATE_FAILED_MALLOC) && defined(__cplusplus)
       void * operator new(
           size_t cb,
           int nBlockUse,
           const char * szFileName,
           int nLine,
           int foo
           );
       void * operator new[](
           size_t cb,
           int nBlockUse,
           const char * szFileName,
           int nLine,
           int foo
           );
       void operator delete(
           void *pUserData,
           int, const char*, int, int
           );
       void operator delete(
           void *pUserData
           );
       void operator delete[]( void * p );
#      define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__, 0)
#    else
#      define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#    endif
#    define new DEBUG_CLIENTBLOCK
#  else
#    define DEBUG_CLIENTBLOCK
#  endif
#endif



#ifndef SK_OVERRIDE
#  if defined(_MSC_VER)
#    define SK_OVERRIDE override
#  elif defined(__clang__)
     
     
     
     
#    pragma clang diagnostic ignored "-Wc++11-extensions"
#
#    if __has_feature(cxx_override_control)
#      define SK_OVERRIDE override
#    elif defined(__has_extension) && __has_extension(cxx_override_control)
#      define SK_OVERRIDE override
#    endif
#  endif
#  ifndef SK_OVERRIDE
#    define SK_OVERRIDE
#  endif
#endif



#if !defined(SK_UNUSED)
#  define SK_UNUSED SK_ATTRIBUTE(unused)
#endif

#if !defined(SK_ATTR_DEPRECATED)
   
#  define SK_ATTR_DEPRECATED(msg) SK_ATTRIBUTE(deprecated)
#endif

#if !defined(SK_ATTR_EXTERNALLY_DEPRECATED)
#  if !defined(SK_INTERNAL)
#    define SK_ATTR_EXTERNALLY_DEPRECATED(msg) SK_ATTR_DEPRECATED(msg)
#  else
#    define SK_ATTR_EXTERNALLY_DEPRECATED(msg)
#  endif
#endif







#if !defined(SK_ALWAYS_INLINE)
#  if defined(SK_BUILD_FOR_WIN)
#    define SK_ALWAYS_INLINE __forceinline
#  else
#    define SK_ALWAYS_INLINE SK_ATTRIBUTE(always_inline) inline
#  endif
#endif



#if defined(__clang__) || defined(__GNUC__)
#  define SK_PREFETCH(ptr) __builtin_prefetch(ptr)
#  define SK_WRITE_PREFETCH(ptr) __builtin_prefetch(ptr, 1)
#else
#  define SK_PREFETCH(ptr)
#  define SK_WRITE_PREFETCH(ptr)
#endif



#ifndef SK_PRINTF_LIKE
#  if defined(__clang__) || defined(__GNUC__)
#    define SK_PRINTF_LIKE(A, B) __attribute__((format(printf, (A), (B))))
#  else
#    define SK_PRINTF_LIKE(A, B)
#  endif
#endif



#ifndef SK_SIZE_T_SPECIFIER
#  if defined(_MSC_VER)
#    define SK_SIZE_T_SPECIFIER "%Iu"
#  else
#    define SK_SIZE_T_SPECIFIER "%zu"
#  endif
#endif



#ifndef SK_ALLOW_STATIC_GLOBAL_INITIALIZERS
#  define SK_ALLOW_STATIC_GLOBAL_INITIALIZERS 1
#endif



#ifndef SK_ATOMICS_PLATFORM_H
#  if defined(_MSC_VER)
#    define SK_ATOMICS_PLATFORM_H "../../src/ports/SkAtomics_win.h"
#  else
#    define SK_ATOMICS_PLATFORM_H "../../src/ports/SkAtomics_sync.h"
#  endif
#endif

#ifndef SK_MUTEX_PLATFORM_H
#  if defined(SK_BUILD_FOR_WIN)
#    define SK_MUTEX_PLATFORM_H "../../src/ports/SkMutex_win.h"
#  else
#    define SK_MUTEX_PLATFORM_H "../../src/ports/SkMutex_pthread.h"
#  endif
#endif

#ifndef SK_BARRIERS_PLATFORM_H
#  if SK_HAS_COMPILER_FEATURE(thread_sanitizer)
#    define SK_BARRIERS_PLATFORM_H "../../src/ports/SkBarriers_tsan.h"
#  elif defined(SK_CPU_ARM32) || defined(SK_CPU_ARM64)
#    define SK_BARRIERS_PLATFORM_H "../../src/ports/SkBarriers_arm.h"
#  else
#    define SK_BARRIERS_PLATFORM_H "../../src/ports/SkBarriers_x86.h"
#  endif
#endif




#if defined(SK_GAMMA_EXPONENT) && defined(SK_GAMMA_SRGB)
#  error "cannot define both SK_GAMMA_EXPONENT and SK_GAMMA_SRGB"
#elif defined(SK_GAMMA_SRGB)
#  define SK_GAMMA_EXPONENT (0.0f)
#elif !defined(SK_GAMMA_EXPONENT)
#  define SK_GAMMA_EXPONENT (2.2f)
#endif

#endif 
