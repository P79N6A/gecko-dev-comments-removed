














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x205
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x205) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#ifdef _STLP_WCE

#  if !defined(__BUILDING_STLPORT) && (_STLP_OUTERMOST_HEADER_ID == 0x205)
#    pragma message("eMbedded Visual C++ 3 and .NET don't have a errno.h header; STLport won't include native errno.h here")
#  endif
#else
#  ifndef errno

#    define _STLP_NATIVE_ERRNO_H_INCLUDED
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <errno.h>
#    else
#      include _STLP_NATIVE_C_HEADER(errno.h)
#    endif
#    if defined (__BORLANDC__) && (__BORLANDC__ >= 0x590) && defined (__cplusplus)
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::__errno;
_STLP_END_NAMESPACE
#    endif
#  endif 

#  if !defined (_STLP_NATIVE_ERRNO_H_INCLUDED)










#    error errno has been defined before inclusion of errno.h header.
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x205)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif





