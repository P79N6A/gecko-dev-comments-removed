














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x201
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x201) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if defined(_STLP_WCE_EVC3)
struct _exception;
#endif

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <limits.h>
#else
#  include _STLP_NATIVE_C_HEADER(limits.h)
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x201)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif
