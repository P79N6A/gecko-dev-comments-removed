


















#ifndef _STLP_STRING_H

#if !defined (RC_INVOKED)

#  ifndef _STLP_OUTERMOST_HEADER_ID
#    define _STLP_OUTERMOST_HEADER_ID 0x269
#    include <stl/_cprolog.h>
#  elif (_STLP_OUTERMOST_HEADER_ID == 0x269) && !defined (_STLP_DONT_POP_HEADER_ID)
#    define _STLP_DONT_POP_HEADER_ID
#    define _STLP_STRING_H
#  endif

#  if defined(_STLP_WCE_EVC3)
struct _exception;
#  endif
#  if (_STLP_OUTERMOST_HEADER_ID != 0x269) || defined (_STLP_DONT_POP_HEADER_ID)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <string.h>
#    else
#      include _STLP_NATIVE_C_HEADER(string.h)
#    endif
#  else
#    if defined (__BORLANDC__) && !defined (__linux__)
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <_str.h>
#      else
#        include _STLP_NATIVE_CPP_C_HEADER(_str.h)
#      endif
#    else
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <string.h>
#      else
#        include _STLP_NATIVE_C_HEADER(string.h)
#      endif
#    endif
#  endif

#  if (_STLP_OUTERMOST_HEADER_ID == 0x269)
#    if !defined (_STLP_DONT_POP_HEADER_ID)
#      include <stl/_epilog.h>
#      undef _STLP_OUTERMOST_HEADER_ID
#    else
#      undef _STLP_DONT_POP_HEADER_ID
#    endif
#  endif
#endif 
#endif 
