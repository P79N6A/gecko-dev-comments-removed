














#ifndef _STLP_INTERNAL_CSTDLIB
#define _STLP_INTERNAL_CSTDLIB

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cstdlib>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cstdlib)
#  endif
#else
#  include <stdlib.h>
#endif

#if defined (__BORLANDC__) && !defined (__linux__)






#  include <process.h>
#endif


#if defined (_STLP_WCE)
#  define _STLP_NATIVE_SETJMP_H_INCLUDED
#endif

#if defined (__MSL__) && (__MSL__ <= 0x5003)
namespace std {
  typedef ::div_t div_t;
  typedef ::ldiv_t ldiv_t;
#  ifdef __MSL_LONGLONG_SUPPORT__
  typedef ::lldiv_t lldiv_t;
#  endif
}
#endif

#ifdef _STLP_IMPORT_VENDOR_CSTD
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::div_t;
using _STLP_VENDOR_CSTD::ldiv_t;
using _STLP_VENDOR_CSTD::size_t;

#  ifndef _STLP_NO_CSTD_FUNCTION_IMPORTS
#    ifndef _STLP_WCE

using _STLP_VENDOR_CSTD::abort;
using _STLP_VENDOR_CSTD::getenv;
using _STLP_VENDOR_CSTD::mblen;
using _STLP_VENDOR_CSTD::mbtowc;
using _STLP_VENDOR_CSTD::system;
using _STLP_VENDOR_CSTD::bsearch;
#    endif
using _STLP_VENDOR_CSTD::atexit;
using _STLP_VENDOR_CSTD::exit;
using _STLP_VENDOR_CSTD::calloc;
using _STLP_VENDOR_CSTD::free;
using _STLP_VENDOR_CSTD::malloc;
using _STLP_VENDOR_CSTD::realloc;
using _STLP_VENDOR_CSTD::atof;
using _STLP_VENDOR_CSTD::atoi;
using _STLP_VENDOR_CSTD::atol;
using _STLP_VENDOR_CSTD::mbstowcs;
using _STLP_VENDOR_CSTD::strtod;
using _STLP_VENDOR_CSTD::strtol;
using _STLP_VENDOR_CSTD::strtoul;

#    if !(defined (_STLP_NO_NATIVE_WIDE_STREAMS) || defined (_STLP_NO_NATIVE_MBSTATE_T))
using _STLP_VENDOR_CSTD::wcstombs;
#      ifndef _STLP_WCE
using _STLP_VENDOR_CSTD::wctomb;
#      endif
#    endif
using _STLP_VENDOR_CSTD::qsort;
using _STLP_VENDOR_CSTD::labs;
using _STLP_VENDOR_CSTD::ldiv;
#    if defined (_STLP_LONG_LONG) && !defined (_STLP_NO_VENDOR_STDLIB_L)
#      if !defined(__sun)
using _STLP_VENDOR_CSTD::llabs;
using _STLP_VENDOR_CSTD::lldiv_t;
using _STLP_VENDOR_CSTD::lldiv;
#      else
using ::llabs;
using ::lldiv_t;
using ::lldiv;
#      endif
#    endif
using _STLP_VENDOR_CSTD::rand;
using _STLP_VENDOR_CSTD::srand;
#  endif 
_STLP_END_NAMESPACE
#endif 

#if (defined (__BORLANDC__) || defined (__WATCOMC__)) && defined (_STLP_USE_NEW_C_HEADERS)


inline int abs(int __x) { return _STLP_VENDOR_CSTD::abs(__x); }
inline _STLP_VENDOR_CSTD::div_t div(int __x, int __y) { return _STLP_VENDOR_CSTD::div(__x, __y); }
#endif

#if defined(_MSC_EXTENSIONS) && defined(_STLP_MSVC) && (_STLP_MSVC <= 1300)
#  define _STLP_RESTORE_FUNCTION_INTRINSIC
#  pragma warning (push)
#  pragma warning (disable: 4162)
#  pragma function (abs)
#endif


#if !defined (__SUNPRO_CC) && \
    (!defined (__HP_aCC) || (__HP_aCC < 30000))


#  if !defined (__WATCOMC__) && \
     (!defined (_STLP_MSVC_LIB) || (_STLP_MSVC_LIB < 1310) || defined (UNDER_CE))
inline long abs(long __x) { return _STLP_VENDOR_CSTD::labs(__x); }
#  endif


#  if !defined (__WATCOMC__) && \
     (!defined (_STLP_MSVC_LIB) || (_STLP_MSVC_LIB < 1400) || defined (_STLP_USING_PLATFORM_SDK_COMPILER) || defined (UNDER_CE))
inline _STLP_VENDOR_CSTD::ldiv_t div(long __x, long __y) { return _STLP_VENDOR_CSTD::ldiv(__x, __y); }
#  endif

#endif

#if defined (_STLP_RESTORE_FUNCTION_INTRINSIC)
#  pragma intrinsic (abs)
#  pragma warning (pop)
#  undef _STLP_RESTORE_FUNCTION_INTRINSIC
#endif

#if defined (_STLP_LONG_LONG)
#  if !defined (_STLP_NO_VENDOR_STDLIB_L)
#    if !defined (__sun)
inline _STLP_LONG_LONG  abs(_STLP_LONG_LONG __x) { return _STLP_VENDOR_CSTD::llabs(__x); }
inline lldiv_t div(_STLP_LONG_LONG __x, _STLP_LONG_LONG __y) { return _STLP_VENDOR_CSTD::lldiv(__x, __y); }
#    else
inline _STLP_LONG_LONG  abs(_STLP_LONG_LONG __x) { return ::llabs(__x); }
inline lldiv_t div(_STLP_LONG_LONG __x, _STLP_LONG_LONG __y) { return ::lldiv(__x, __y); }
#    endif
#  else
inline _STLP_LONG_LONG  abs(_STLP_LONG_LONG __x) { return __x < 0 ? -__x : __x; }
#  endif
#endif







#ifndef _STLP_INTERNAL_CMATH
#  include <stl/_cmath.h>
#endif

#if defined (_STLP_IMPORT_VENDOR_CSTD) && !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)

_STLP_BEGIN_NAMESPACE
using ::abs;
using ::div;
_STLP_END_NAMESPACE
#endif

#endif 
