
















#include "stlport_prefix.h"

#include <sstream>

_STLP_BEGIN_NAMESPACE

#if !defined (_STLP_NO_FORCE_INSTANTIATE)


template class _STLP_CLASS_DECLSPEC basic_stringbuf<char, char_traits<char>, allocator<char> >;
template class _STLP_CLASS_DECLSPEC basic_ostringstream<char, char_traits<char>, allocator<char> >;
template class _STLP_CLASS_DECLSPEC basic_istringstream<char, char_traits<char>, allocator<char> >;
template class _STLP_CLASS_DECLSPEC basic_stringstream<char, char_traits<char>, allocator<char> >;

#  if !defined (_STLP_NO_WCHAR_T)
template class _STLP_CLASS_DECLSPEC basic_stringbuf<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
template class _STLP_CLASS_DECLSPEC basic_ostringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
template class _STLP_CLASS_DECLSPEC basic_istringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
template class _STLP_CLASS_DECLSPEC basic_stringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
#  endif

#endif

_STLP_END_NAMESPACE




