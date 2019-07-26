














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x279
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x279) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif


#if !defined(_STLP_WCE_EVC3)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    if defined (__hpux)
#      include_next <stdarg.h>
#      include_next <wchar.h>
#    endif
#    include_next <wctype.h>
#  else
#    if defined (__hpux)
#      include _STLP_NATIVE_C_HEADER(stdarg.h)
#      include _STLP_NATIVE_C_HEADER(wchar.h)
#    endif
#    include _STLP_NATIVE_C_HEADER(wctype.h)
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x279)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
