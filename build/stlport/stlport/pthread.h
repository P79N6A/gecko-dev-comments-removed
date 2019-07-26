














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x280
#  include <stl/_prolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x280) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if defined (__SUNPRO_CC) || defined (__HP_aCC)
#  include "/usr/include/pthread.h"
#else
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <pthread.h>
#  else
#    include _STLP_NATIVE_C_HEADER(pthread.h)
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x280)
#  if !defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif






