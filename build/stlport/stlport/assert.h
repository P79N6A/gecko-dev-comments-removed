














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x202
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x202) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif


#ifndef _STLP_WCE_EVC3
#  if !defined (assert)
#    define _STLP_NATIVE_ASSERT_H_INCLUDED
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <assert.h>
#    else
#      include _STLP_NATIVE_C_HEADER(assert.h)
#    endif
#  endif
#  if !defined (_STLP_NATIVE_ASSERT_H_INCLUDED)

#    error assert has been defined before inclusion of assert.h header.
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x202)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif





