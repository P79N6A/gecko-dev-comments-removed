














#ifndef _STLP_INTERNAL_CSIGNAL
#define _STLP_INTERNAL_CSIGNAL

#if !defined (_STLP_WCE)
#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <csignal>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(csignal)
#    endif
#  else
#    include <signal.h>
#  endif

#  if defined (_STLP_IMPORT_VENDOR_CSTD)
_STLP_BEGIN_NAMESPACE
#    if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
using _STLP_VENDOR_CSTD::signal;
using _STLP_VENDOR_CSTD::raise;
#    endif 
using _STLP_VENDOR_CSTD::sig_atomic_t;
_STLP_END_NAMESPACE
#  endif 
#endif

#endif 
