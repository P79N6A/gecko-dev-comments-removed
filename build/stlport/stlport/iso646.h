














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x204
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x204) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif


#if !defined (_STLP_WCE_EVC3) && !defined (__BORLANDC__)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <iso646.h>
#  else
#    include _STLP_NATIVE_C_HEADER(iso646.h)
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x204)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif

