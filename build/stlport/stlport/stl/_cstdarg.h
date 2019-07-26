













#ifndef _STLP_INTERNAL_CSTDARG
#define _STLP_INTERNAL_CSTDARG

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cstdarg>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cstdarg)
#  endif
#else
#  include <stdarg.h>
#endif

#ifdef _STLP_IMPORT_VENDOR_CSTD
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::va_list;
_STLP_END_NAMESPACE
#endif 

#endif
