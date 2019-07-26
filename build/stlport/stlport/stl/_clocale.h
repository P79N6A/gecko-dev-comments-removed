














#ifndef _STLP_INTERNAL_CLOCALE
#define _STLP_INTERNAL_CLOCALE

#if !defined (_STLP_WCE_EVC3)

#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <clocale>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(clocale)
#    endif
#  else
#    include <locale.h>
#  endif

#  if defined (_STLP_IMPORT_VENDOR_CSTD)
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::lconv;
#    if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS) && !defined(__ANDROID__)
using _STLP_VENDOR_CSTD::localeconv;
using _STLP_VENDOR_CSTD::setlocale;
#    endif
_STLP_END_NAMESPACE
#  endif

#endif 

#endif
