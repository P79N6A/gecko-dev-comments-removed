











#if defined (__GNUC__)
#  define FUNC     ((const char*) (__PRETTY_FUNCTION__))
#elif defined (__sun) || (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#  define FUNC     ((const char*) (__func__))
#else
#  define FUNC     ((const char*) ("???"))
#endif

#ifndef INT16_MIN
# define INT16_MIN              (-32767-1)
#endif

#ifndef INT16_MAX
# define INT16_MAX              (32767)
#endif

#ifndef INT32_MIN
# define INT32_MIN              (-2147483647-1)
#endif

#ifndef INT32_MAX
# define INT32_MAX              (2147483647)
#endif

#ifndef UINT32_MIN
# define UINT32_MIN             (0)
#endif

#ifndef UINT32_MAX
# define UINT32_MAX             (4294967295U)
#endif

#ifndef M_PI
# define M_PI			3.14159265358979323846
#endif

#ifdef _MSC_VER

#   define inline __inline
#   define force_inline __forceinline
#elif defined __GNUC__ || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#   define inline __inline__
#   define force_inline __inline__ __attribute__ ((__always_inline__))
#else
#   ifndef force_inline
#      define force_inline inline
#   endif
#endif


#if defined(__GNUC__) && __GNUC__ >= 4
#   define PIXMAN_EXPORT __attribute__ ((visibility("default")))

#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#   define PIXMAN_EXPORT __global
#else
#   define PIXMAN_EXPORT
#endif

