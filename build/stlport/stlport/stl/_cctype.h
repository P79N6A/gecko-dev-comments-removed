














#ifndef _STLP_INTERNAL_CCTYPE
#define _STLP_INTERNAL_CCTYPE

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cctype>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cctype)
#  endif
#else
#  include <ctype.h>
#endif 

#if ! defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
#  if defined ( _STLP_IMPORT_VENDOR_CSTD )
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::isalnum;
using _STLP_VENDOR_CSTD::isalpha;
using _STLP_VENDOR_CSTD::iscntrl;
using _STLP_VENDOR_CSTD::isdigit;
using _STLP_VENDOR_CSTD::isgraph;
using _STLP_VENDOR_CSTD::islower;
using _STLP_VENDOR_CSTD::isprint;
using _STLP_VENDOR_CSTD::ispunct;
using _STLP_VENDOR_CSTD::isspace;
using _STLP_VENDOR_CSTD::isupper;
using _STLP_VENDOR_CSTD::isxdigit;
using _STLP_VENDOR_CSTD::tolower;
using _STLP_VENDOR_CSTD::toupper;
_STLP_END_NAMESPACE
#  endif 
#endif 

#endif
