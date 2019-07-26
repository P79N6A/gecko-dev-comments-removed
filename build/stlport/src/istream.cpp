
















# include "stlport_prefix.h"

#include <istream>

_STLP_BEGIN_NAMESPACE

#if !defined(_STLP_NO_FORCE_INSTANTIATE)


#  if defined (_STLP_USE_TEMPLATE_EXPORT)
template class _STLP_CLASS_DECLSPEC _Isentry<char, char_traits<char> >;
#  endif

template class _STLP_CLASS_DECLSPEC basic_iostream<char, char_traits<char> >;
template class _STLP_CLASS_DECLSPEC basic_istream<char, char_traits<char> >;

#  if !defined (_STLP_NO_WCHAR_T)
#    if defined (_STLP_USE_TEMPLATE_EXPORT)
template class _STLP_CLASS_DECLSPEC _Isentry<wchar_t, char_traits<wchar_t> >;
#    endif
template class _STLP_CLASS_DECLSPEC basic_istream<wchar_t, char_traits<wchar_t> >;
template class _STLP_CLASS_DECLSPEC basic_iostream<wchar_t, char_traits<wchar_t> >;
#  endif 

#endif 

_STLP_END_NAMESPACE




