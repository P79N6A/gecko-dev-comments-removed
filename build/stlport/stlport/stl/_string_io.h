

















#ifndef _STLP_STRING_IO_H
#define _STLP_STRING_IO_H

#ifndef _STLP_INTERNAL_OSTREAM_H
#  include <stl/_ostream.h>
#endif

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h>
#endif


_STLP_BEGIN_NAMESPACE

template <class _CharT, class _Traits, class _Alloc>
basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const basic_string<_CharT,_Traits,_Alloc>& __s);

#if defined (_STLP_USE_TEMPLATE_EXPRESSION)

template <class _CharT, class _Traits, class _Alloc, class _Left, class _Right, class _StorageDir>
basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const _STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir>& __sum) {
  basic_string<_CharT, _Traits, _Alloc> __tmp(__sum);
  return __os << __tmp;
}

#endif 

template <class _CharT, class _Traits, class _Alloc>
basic_istream<_CharT, _Traits>&  _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is,
           basic_string<_CharT,_Traits,_Alloc>& __s);

template <class _CharT, class _Traits, class _Alloc>
basic_istream<_CharT, _Traits>& _STLP_CALL
getline(basic_istream<_CharT, _Traits>& __is,
        basic_string<_CharT,_Traits,_Alloc>& __s,
        _CharT __delim);

#if !(defined (__BORLANDC__) && !defined (_STLP_USE_OWN_NAMESPACE))

template <class _CharT, class _Traits, class _Alloc>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
getline(basic_istream<_CharT, _Traits>& __is,
        basic_string<_CharT,_Traits,_Alloc>& __s) {
  return getline(__is, __s, __is.widen('\n'));
}
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_string_io.c>
#endif

#endif 
