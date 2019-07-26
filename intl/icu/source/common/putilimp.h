














#ifndef PUTILIMP_H
#define PUTILIMP_H

#include "unicode/utypes.h"
#include "unicode/putil.h"






















#ifdef U_SIGNED_RIGHT_SHIFT_IS_ARITHMETIC
    
#else
    



#   define U_SIGNED_RIGHT_SHIFT_IS_ARITHMETIC 1
#endif



#ifndef IEEE_754
#   define IEEE_754 1
#endif











#if !defined(__intptr_t_defined) && !defined(UINTPTR_MAX) && (U_PLATFORM != U_PF_OS390)
typedef size_t uintptr_t;
#endif






#if !defined(U_HAVE_MSVC_2003_OR_EARLIER) && defined(_MSC_VER) && (_MSC_VER < 1400)
#define U_HAVE_MSVC_2003_OR_EARLIER
#endif





#ifdef U_HAVE_NL_LANGINFO_CODESET
    
#elif U_PLATFORM_HAS_WIN32_API
#   define U_HAVE_NL_LANGINFO_CODESET 0
#else
#   define U_HAVE_NL_LANGINFO_CODESET 1
#endif

#ifdef U_NL_LANGINFO_CODESET
    
#elif !U_HAVE_NL_LANGINFO_CODESET
#   define U_NL_LANGINFO_CODESET -1
#elif U_PLATFORM == U_PF_OS400
   
#else
#   define U_NL_LANGINFO_CODESET CODESET
#endif

#ifdef U_TZSET
    
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   define U_TZSET _tzset
#elif U_PLATFORM == U_PF_OS400
   
#else
#   define U_TZSET tzset
#endif

#ifdef U_TIMEZONE
    
#elif U_PLATFORM == U_PF_ANDROID
#   define U_TIMEZONE timezone
#elif U_PLATFORM_IS_LINUX_BASED
#   define U_TIMEZONE __timezone
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   define U_TIMEZONE _timezone
#elif U_PLATFORM == U_PF_BSD && !defined(__NetBSD__)
   
#elif U_PLATFORM == U_PF_OS400
   
#else
#   define U_TIMEZONE timezone
#endif

#ifdef U_TZNAME
    
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   define U_TZNAME _tzname
#elif U_PLATFORM == U_PF_OS400
   
#else
#   define U_TZNAME tzname
#endif

#ifdef U_HAVE_MMAP
    
#elif U_PLATFORM_HAS_WIN32_API
#   define U_HAVE_MMAP 0
#else
#   define U_HAVE_MMAP 1
#endif

#ifdef U_HAVE_POPEN
    
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   define U_HAVE_POPEN 0
#elif U_PLATFORM == U_PF_OS400
#   define U_HAVE_POPEN 0
#else
#   define U_HAVE_POPEN 1
#endif






#ifdef U_HAVE_DIRENT_H
    
#elif U_PLATFORM_HAS_WIN32_API
#   define U_HAVE_DIRENT_H 0
#else
#   define U_HAVE_DIRENT_H 1
#endif











#ifdef U_HAVE_GCC_ATOMICS
    
#elif U_GCC_MAJOR_MINOR >= 404
#   define U_HAVE_GCC_ATOMICS 1
#else
#   define U_HAVE_GCC_ATOMICS 0
#endif













#ifdef U_ALIGN_CODE
    
#elif defined(_MSC_VER) && defined(_M_IX86) && !defined(_MANAGED)
#   define U_ALIGN_CODE(boundarySize) __asm  align boundarySize
#else
#   define U_ALIGN_CODE(boundarySize) 
#endif











#ifdef U_MAKE_IS_NMAKE
    
#elif U_PLATFORM == U_PF_WINDOWS
#   define U_MAKE_IS_NMAKE 1
#else
#   define U_MAKE_IS_NMAKE 0
#endif

















U_INTERNAL UBool   U_EXPORT2 uprv_isNaN(double d);




U_INTERNAL UBool   U_EXPORT2 uprv_isInfinite(double d);




U_INTERNAL UBool   U_EXPORT2 uprv_isPositiveInfinity(double d);




U_INTERNAL UBool   U_EXPORT2 uprv_isNegativeInfinity(double d);




U_INTERNAL double  U_EXPORT2 uprv_getNaN(void);




U_INTERNAL double  U_EXPORT2 uprv_getInfinity(void);





U_INTERNAL double  U_EXPORT2 uprv_trunc(double d);




U_INTERNAL double  U_EXPORT2 uprv_floor(double d);




U_INTERNAL double  U_EXPORT2 uprv_ceil(double d);




U_INTERNAL double  U_EXPORT2 uprv_fabs(double d);




U_INTERNAL double  U_EXPORT2 uprv_modf(double d, double* pinteger);




U_INTERNAL double  U_EXPORT2 uprv_fmod(double d, double y);




U_INTERNAL double  U_EXPORT2 uprv_pow(double d, double exponent);




U_INTERNAL double  U_EXPORT2 uprv_pow10(int32_t exponent);




U_INTERNAL double  U_EXPORT2 uprv_fmax(double d, double y);




U_INTERNAL double  U_EXPORT2 uprv_fmin(double d, double y);




U_INTERNAL int32_t U_EXPORT2 uprv_max(int32_t d, int32_t y);




U_INTERNAL int32_t U_EXPORT2 uprv_min(int32_t d, int32_t y);

#if U_IS_BIG_ENDIAN
#   define uprv_isNegative(number) (*((signed char *)&(number))<0)
#else
#   define uprv_isNegative(number) (*((signed char *)&(number)+sizeof(number)-1)<0)
#endif






U_INTERNAL double  U_EXPORT2 uprv_maxMantissa(void);





U_INTERNAL double  U_EXPORT2 uprv_log(double d);







U_INTERNAL double  U_EXPORT2 uprv_round(double x);

#if 0








#endif

#if !U_CHARSET_IS_UTF8








U_INTERNAL const char*  U_EXPORT2 uprv_getDefaultCodepage(void);
#endif










U_INTERNAL const char*  U_EXPORT2 uprv_getDefaultLocaleID(void);

































U_INTERNAL void     U_EXPORT2 uprv_tzset(void);







U_INTERNAL int32_t  U_EXPORT2 uprv_timezone(void);









U_INTERNAL const char* U_EXPORT2 uprv_tzname(int n);







U_INTERNAL UDate U_EXPORT2 uprv_getUTCtime(void);








U_INTERNAL UDate U_EXPORT2 uprv_getRawUTCtime(void);







U_INTERNAL UBool U_EXPORT2 uprv_pathIsAbsolute(const char *path);







U_INTERNAL void * U_EXPORT2 uprv_maximumPtr(void *base);















#ifndef U_MAX_PTR
#  if U_PLATFORM == U_PF_OS390 && !defined(_LP64)
    
#    define U_MAX_PTR(base) ((void *)0x7fffffff)
#  elif U_PLATFORM == U_PF_OS400
#    define U_MAX_PTR(base) uprv_maximumPtr((void *)base)
#  elif 0
    









#    define U_MAX_PTR(base) \
    ((void *)(((char *)(base)+0x7fffffffu) > (char *)(base) \
        ? ((char *)(base)+0x7fffffffu) \
        : (char *)-1))
#  else
    
#    define U_MAX_PTR(base) \
    ((void *)(((uintptr_t)(base)+0x7fffffffu) > (uintptr_t)(base) \
        ? ((uintptr_t)(base)+0x7fffffffu) \
        : (uintptr_t)-1))
#  endif
#endif



typedef void (UVoidFunction)(void);

#if U_ENABLE_DYLOAD




U_INTERNAL void * U_EXPORT2 uprv_dl_open(const char *libName, UErrorCode *status);





U_INTERNAL void U_EXPORT2 uprv_dl_close( void *lib, UErrorCode *status);





U_INTERNAL UVoidFunction* U_EXPORT2 uprv_dlsym_func( void *lib, const char *symbolName, UErrorCode *status);








#endif





#if U_PLATFORM == U_PF_OS400
# define uprv_default_malloc(x) _C_TS_malloc(x)
# define uprv_default_realloc(x,y) _C_TS_realloc(x,y)
# define uprv_default_free(x) _C_TS_free(x)

#else

# define uprv_default_malloc(x) malloc(x)
# define uprv_default_realloc(x,y) realloc(x,y)
# define uprv_default_free(x) free(x)
#endif


#endif
