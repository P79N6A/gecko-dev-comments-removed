
















#include "stlport_prefix.h"

#include <ostream>

_STLP_BEGIN_NAMESPACE

#if !defined(_STLP_NO_FORCE_INSTANTIATE)


template class _STLP_CLASS_DECLSPEC basic_ostream<char, char_traits<char> >;

# if defined (_STLP_USE_TEMPLATE_EXPORT)
template  class _STLP_CLASS_DECLSPEC _Osentry<char, char_traits<char> >;
# endif

#ifndef _STLP_NO_WCHAR_T

# if defined (_STLP_USE_TEMPLATE_EXPORT)
template class _STLP_CLASS_DECLSPEC _Osentry<wchar_t, char_traits<wchar_t> >;
# endif
template class _STLP_CLASS_DECLSPEC basic_ostream<wchar_t, char_traits<wchar_t> >;
#endif

#endif

_STLP_END_NAMESPACE




