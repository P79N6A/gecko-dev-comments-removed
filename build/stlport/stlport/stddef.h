














#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x262
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x262) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if defined (_MSC_VER) || defined (__DMC__)




#  include "errno.h"
#endif

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <stddef.h>
#else
#  include _STLP_NATIVE_C_HEADER(stddef.h)
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x262)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
