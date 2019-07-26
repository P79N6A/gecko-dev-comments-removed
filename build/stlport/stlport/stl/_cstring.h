














#ifndef _STLP_INTERNAL_CSTRING
#define _STLP_INTERNAL_CSTRING

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cstring>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cstring)
#  endif
#else
#  include <string.h>
#endif

#ifdef _STLP_IMPORT_VENDOR_CSTD
_STLP_BEGIN_NAMESPACE
#  include <using/cstring>
_STLP_END_NAMESPACE
#endif 

#endif 
