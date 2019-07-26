
















#ifndef _STLP_COMPLEX_C
#define _STLP_COMPLEX_C

#ifndef _STLP_INTERNAL_COMPLEX
#  include <stl/_complex.h>
#endif

#if !defined (_STLP_USE_NO_IOSTREAMS)
#  ifndef _STLP_INTERNAL_ISTREAM
#    include <stl/_istream.h>
#  endif

#  ifndef _STLP_INTERNAL_SSTREAM
#    include <stl/_sstream.h>
#  endif

#  ifndef _STLP_STRING_IO_H
#    include <stl/_string_io.h>
#  endif
#endif

_STLP_BEGIN_NAMESPACE



template <class _Tp>
void complex<_Tp>::_div(const _Tp& __z1_r, const _Tp& __z1_i,
                        const _Tp& __z2_r, const _Tp& __z2_i,
                        _Tp& __res_r, _Tp& __res_i) {
  _Tp __ar = __z2_r >= 0 ? __z2_r : -__z2_r;
  _Tp __ai = __z2_i >= 0 ? __z2_i : -__z2_i;

  if (__ar <= __ai) {
    _Tp __ratio = __z2_r / __z2_i;
    _Tp __denom = __z2_i * (1 + __ratio * __ratio);
    __res_r = (__z1_r * __ratio + __z1_i) / __denom;
    __res_i = (__z1_i * __ratio - __z1_r) / __denom;
  }
  else {
    _Tp __ratio = __z2_i / __z2_r;
    _Tp __denom = __z2_r * (1 + __ratio * __ratio);
    __res_r = (__z1_r + __z1_i * __ratio) / __denom;
    __res_i = (__z1_i - __z1_r * __ratio) / __denom;
  }
}

template <class _Tp>
void complex<_Tp>::_div(const _Tp& __z1_r,
                        const _Tp& __z2_r, const _Tp& __z2_i,
                        _Tp& __res_r, _Tp& __res_i) {
  _Tp __ar = __z2_r >= 0 ? __z2_r : -__z2_r;
  _Tp __ai = __z2_i >= 0 ? __z2_i : -__z2_i;

  if (__ar <= __ai) {
    _Tp __ratio = __z2_r / __z2_i;
    _Tp __denom = __z2_i * (1 + __ratio * __ratio);
    __res_r = (__z1_r * __ratio) / __denom;
    __res_i = - __z1_r / __denom;
  }
  else {
    _Tp __ratio = __z2_i / __z2_r;
    _Tp __denom = __z2_r * (1 + __ratio * __ratio);
    __res_r = __z1_r / __denom;
    __res_i = - (__z1_r * __ratio) / __denom;
  }
}


#if !defined (_STLP_USE_NO_IOSTREAMS)



template <class _Tp, class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, const complex<_Tp>& __z) {
  basic_ostringstream<_CharT, _Traits, allocator<_CharT> > __tmp;
  __tmp.flags(__os.flags());
  __tmp.imbue(__os.getloc());
  __tmp.precision(__os.precision());
  __tmp << '(' << __z.real() << ',' << __z.imag() << ')';
  return __os << __tmp.str();
}





template <class _Tp, class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is, complex<_Tp>& __z) {
  _Tp  __re = 0;
  _Tp  __im = 0;

  const ctype<_CharT>& __c_type = *__is._M_ctype_facet();

  const char __punct[4] = "(,)";
  _CharT __wpunct[3];
  __c_type.widen(__punct, __punct + 3, __wpunct);

  _CharT __c;

  __is >> __c;
  if (_Traits::eq(__c, __wpunct[0])) {  
    __is >> __re >> __c;
    if (_Traits::eq(__c, __wpunct[1]))  
      __is >> __im >> __c;
    if (!_Traits::eq(__c, __wpunct[2])) 
      __is.setstate(ios_base::failbit);
  }
  else {
    __is.putback(__c);
    __is >> __re;
  }

  if (__is)
    __z = complex<_Tp>(__re, __im);
  return __is;
}

#endif 

_STLP_END_NAMESPACE

#endif 




