

















#ifndef _STLP_STRING_FWD_H
#define _STLP_STRING_FWD_H

#ifndef _STLP_INTERNAL_IOSFWD
#  include <stl/_iosfwd.h>
#endif

_STLP_BEGIN_NAMESPACE

#if !defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _CharT,
          class _Traits = char_traits<_CharT>,
          class _Alloc = allocator<_CharT> >
class basic_string;
#else
template <class _CharT,
          class _Traits,
          class _Alloc>
class basic_string;
#endif 

typedef basic_string<char, char_traits<char>, allocator<char> > string;

#if defined (_STLP_HAS_WCHAR_T)
typedef basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > wstring;
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE



const char* _STLP_CALL __get_c_string(const string& __str);

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif 




