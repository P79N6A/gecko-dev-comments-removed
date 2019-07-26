














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x242
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x242)
#  if !defined (_STLP_DONT_POP_HEADER_ID)
#    define _STLP_DONT_POP_HEADER_ID
#  else
#    error STLport include schema violation
#  endif
#endif


#ifndef _STLP_WCE_EVC3
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <locale.h>
#  else
#    include _STLP_NATIVE_C_HEADER(locale.h)
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x242)
#  if !defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
