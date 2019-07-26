














#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x203
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x203) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#elif (_STLP_OUTERMOST_HEADER_ID == 0x203)
#  error This header is only reentrant once, it should be modified if it has to be included more.
#endif

#if defined (_STLP_WCE_EVC3)
struct _exception;
#endif

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <float.h>
#else
#  include _STLP_NATIVE_C_HEADER(float.h)
#endif

#if !defined (__linux__)
#  if defined(__BORLANDC__) && defined (__cplusplus) && (__BORLANDC__ >= 0x560)
_STLP_BEGIN_NAMESPACE
using ::_max_dble;
using ::_max_flt;
using ::_max_ldble;
using ::_tiny_ldble;
_STLP_END_NAMESPACE
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x203)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif
