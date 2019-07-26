














#ifndef _STLP_mem_h

#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x245
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x245) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if (_STLP_OUTERMOST_HEADER_ID != 0x245) || defined (_STLP_DONT_POP_HEADER_ID)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <mem.h>
#  else
#    include _STLP_NATIVE_C_HEADER(mem.h)
#  endif
#else
#  if defined (__BORLANDC__) && defined (__USING_CNAME__)
#    define _USING_CNAME_WAS_UNDEFINED
#    undef __USING_CNAME__
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <mem.h>
#  else
#    include _STLP_NATIVE_C_HEADER(mem.h)
#  endif

#  if defined (__BORLANDC__) && defined (_USING_CNAME_WAS_UNDEFINED)
#    define __USING_CNAME__
#    define _STLP_mem_h 1
#    undef _USING_CNAME_WAS_UNDEFINED
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x245)
#  if !defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif

#endif 
