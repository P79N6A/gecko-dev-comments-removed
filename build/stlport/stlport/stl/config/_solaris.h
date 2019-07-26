#ifndef __stl_config__solaris_h
#define __stl_config__solaris_h

#define _STLP_PLATFORM "Sun Solaris"


#include <sys/feature_tests.h>


#define _STLP_USE_UNIX_IO

#ifdef __GNUC__



#  if !(defined(__SunOS_5_5_1) || defined(__SunOS_5_6) || defined(__SunOS_5_7) || \
        defined(__SunOS_5_8) || defined(__SunOS_5_9) || defined(__SunOS_5_10))
#    error Uncomment one of the defines (__SunOS_5_x) in the file stlport/stl/config/host.h
#  endif
#endif

#if defined (__SunOS_5_8) && ! defined (_STLP_HAS_NO_NEW_C_HEADERS) && ( __cplusplus >= 199711L)
#  define _STLP_HAS_NATIVE_FLOAT_ABS
#endif

#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
# define _STLP_RAND48 1
#endif

#if (defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 == 4)) || defined (__SunOS_5_6)
# define _STLP_WCHAR_SUNPRO_EXCLUDE 1
# define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#endif


#if !(defined ( __KCC ) && __KCC_VERSION > 3400 ) && \
  ((defined(__SunOS_5_5_1) || defined(__SunOS_5_6) ))
#  ifndef _STLP_NO_NATIVE_MBSTATE_T
#    define _STLP_NO_NATIVE_MBSTATE_T 1
#  endif
#endif 


#if defined (__sparc) 
#  if ( (defined (__GNUC__) && defined (__sparc_v9__)) || \
        defined (__sparcv9) ) \
       && !defined(_NOTHREADS) && !defined (_STLP_NO_SPARC_SOLARIS_THREADS)
#    define _STLP_SPARC_SOLARIS_THREADS
#    define _STLP_THREADS_DEFINED
#  endif
#endif




#ifdef __GNUC__
#  if (defined  (__sparc_v9__) || defined (__sparcv9)) && !defined ( __WORD64 ) && !defined(__arch64__)
#    define __LONG_MAX__ 2147483647L
#  endif
#endif






#if !defined(__SunOS_5_10) && !defined(_STLP_SOLARIS_MATH_PATCH)
#  define _STLP_NO_VENDOR_MATH_F
#  define _STLP_NO_VENDOR_MATH_L
#endif

#ifdef __GNUC__
#  define _STLP_WCHAR_BORLAND_EXCLUDE
#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#endif

#endif 
