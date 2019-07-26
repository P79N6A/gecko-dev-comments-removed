














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x272
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x272) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#ifdef _STLP_WCE_EVC3

#  if !defined(__BUILDING_STLPORT) && (_STLP_OUTERMOST_HEADER_ID == 0x272)
#    pragma message("eMbedded Visual C++ 3 doesn't have a time.h header; STLport won't include native time.h here")
#  endif
#else
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <time.h>
#  else
#    include _STLP_NATIVE_C_HEADER(time.h)
#  endif
#endif


#if (_STLP_OUTERMOST_HEADER_ID == 0x272)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
