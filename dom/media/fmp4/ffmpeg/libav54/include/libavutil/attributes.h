
























#ifndef AVUTIL_ATTRIBUTES_H
#define AVUTIL_ATTRIBUTES_H

#ifdef __GNUC__
#    define AV_GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > x || __GNUC__ == x && __GNUC_MINOR__ >= y)
#else
#    define AV_GCC_VERSION_AT_LEAST(x,y) 0
#endif

#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define av_always_inline __forceinline
#else
#    define av_always_inline inline
#endif

#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_noinline __attribute__((noinline))
#else
#    define av_noinline
#endif

#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_pure __attribute__((pure))
#else
#    define av_pure
#endif

#if AV_GCC_VERSION_AT_LEAST(2,6)
#    define av_const __attribute__((const))
#else
#    define av_const
#endif

#if AV_GCC_VERSION_AT_LEAST(4,3)
#    define av_cold __attribute__((cold))
#else
#    define av_cold
#endif

#if AV_GCC_VERSION_AT_LEAST(4,1)
#    define av_flatten __attribute__((flatten))
#else
#    define av_flatten
#endif

#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define attribute_deprecated __attribute__((deprecated))
#else
#    define attribute_deprecated
#endif

#if defined(__GNUC__)
#    define av_unused __attribute__((unused))
#else
#    define av_unused
#endif






#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_used __attribute__((used))
#else
#    define av_used
#endif

#if AV_GCC_VERSION_AT_LEAST(3,3)
#   define av_alias __attribute__((may_alias))
#else
#   define av_alias
#endif

#if defined(__GNUC__) && !defined(__ICC)
#    define av_uninit(x) x=x
#else
#    define av_uninit(x) x
#endif

#ifdef __GNUC__
#    define av_builtin_constant_p __builtin_constant_p
#    define av_printf_format(fmtpos, attrpos) __attribute__((__format__(__printf__, fmtpos, attrpos)))
#else
#    define av_builtin_constant_p(x) 0
#    define av_printf_format(fmtpos, attrpos)
#endif

#if AV_GCC_VERSION_AT_LEAST(2,5)
#    define av_noreturn __attribute__((noreturn))
#else
#    define av_noreturn
#endif

#endif 
