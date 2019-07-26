














#ifndef _STLP_INTERNAL_CSTDDEF
#define _STLP_INTERNAL_CSTDDEF

#  if (__GNUC__ >= 3) && defined (__CYGWIN__) 
#    define __need_wint_t
#    define __need_wchar_t
#    define __need_size_t
#    define __need_ptrdiff_t
#    define __need_NULL
#  endif

#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <cstddef>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(cstddef)
#    endif
#  else
#    include <stddef.h>
#  endif

#  ifdef _STLP_IMPORT_VENDOR_CSTD
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::ptrdiff_t;
using _STLP_VENDOR_CSTD::size_t;
_STLP_END_NAMESPACE
#  endif 

#endif 
