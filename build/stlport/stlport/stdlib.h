


















#if !defined (RC_INVOKED)

#  if !defined (_STLP_OUTERMOST_HEADER_ID)
#    define _STLP_OUTERMOST_HEADER_ID 0x265
#    include <stl/_cprolog.h>
#  elif (_STLP_OUTERMOST_HEADER_ID == 0x265) && !defined (_STLP_DONT_POP_HEADER_ID)
#    define _STLP_DONT_POP_HEADER_ID
#  endif

#  if defined (_STLP_MSVC_LIB) || (defined (__GNUC__) && defined (__MINGW32__)) || \
       defined (__BORLANDC__) || defined (__DMC__) || \
       (defined (__HP_aCC) && defined (_REENTRANT))




#    include "errno.h"
#  endif





#  if defined(_STLP_WCE_EVC3)
struct _exception;
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <stdlib.h>
#  else
#    include _STLP_NATIVE_C_HEADER(stdlib.h)
#  endif


#  if defined (_STLP_WCE)
#    define _STLP_NATIVE_SETJMP_H_INCLUDED
#  endif

#  if (_STLP_OUTERMOST_HEADER_ID == 0x265)
#    if ! defined (_STLP_DONT_POP_HEADER_ID)
#      include <stl/_epilog.h>
#      undef  _STLP_OUTERMOST_HEADER_ID
#    else
#      undef  _STLP_DONT_POP_HEADER_ID
#    endif
#  endif

#endif 
