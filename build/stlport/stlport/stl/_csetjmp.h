














#ifndef _STLP_INTERNAL_CSETJMP
#define _STLP_INTERNAL_CSETJMP


#if !defined (setjmp)
#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <csetjmp>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(csetjmp)
#    endif
#  else
#    include <setjmp.h>
#  endif
#endif

#if defined (_STLP_IMPORT_VENDOR_CSTD)

#  if defined (__BORLANDC__) && defined (_STLP_USE_NEW_C_HEADERS)



#    undef _STLP_NATIVE_SETJMP_H_INCLUDED
#  endif

_STLP_BEGIN_NAMESPACE
#  if !defined (_STLP_NATIVE_SETJMP_H_INCLUDED)
using _STLP_VENDOR_CSTD::jmp_buf;
#  else


using ::jmp_buf;
#  endif
#  if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
#    if !defined (setjmp)
#      if !defined (__MSL__) || ((__MSL__ > 0x7001) && (__MSL__ < 0x8000))
#        ifndef _STLP_NATIVE_SETJMP_H_INCLUDED
using _STLP_VENDOR_CSTD::setjmp;
#        else
using ::setjmp;
#        endif
#      endif
#    endif
#    if !defined (_STLP_NATIVE_SETJMP_H_INCLUDED)
using _STLP_VENDOR_CSTD::longjmp;
#    else
using ::longjmp;
#    endif
#  endif
_STLP_END_NAMESPACE
#endif 

#endif
