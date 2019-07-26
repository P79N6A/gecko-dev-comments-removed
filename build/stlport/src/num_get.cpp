
















#include "stlport_prefix.h"

#include <locale>
#include <istream>
#include <algorithm>

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE











bool  _STLP_CALL
__valid_grouping(const char * first1, const char * last1,
                 const char * first2, const char * last2) {
  if (first1 == last1 || first2 == last2) return true;

  --last1; --last2;

  while (first1 != last1) {
    if (*last1 != *first2)
      return false;
    --last1;
    if (first2 != last2) ++first2;
  }

  return *last1 <= *first2;
}

_STLP_DECLSPEC unsigned char _STLP_CALL __digit_val_table(unsigned __index) {
  static const unsigned char __val_table[128] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,10,11,12,13,14,15,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,10,11,12,13,14,15,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
  };

  return __val_table[__index];
}

_STLP_DECLSPEC const char* _STLP_CALL __narrow_atoms()
{ return "+-0xX"; }



#if !defined (_STLP_NO_WCHAR_T)



bool _STLP_CALL __get_fdigit(wchar_t& c, const wchar_t* digits) {
  const wchar_t* p = find(digits, digits + 10, c);
  if (p != digits + 10) {
    c = (char)('0' + (p - digits));
    return true;
  }
  else
    return false;
}

bool _STLP_CALL __get_fdigit_or_sep(wchar_t& c, wchar_t sep,
                                    const wchar_t * digits) {
  if (c == sep) {
    c = (char)',';
    return true;
  }
  else
    return __get_fdigit(c, digits);
}

#endif

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined(_STLP_NO_FORCE_INSTANTIATE)


template class _STLP_CLASS_DECLSPEC istreambuf_iterator<char, char_traits<char> >;

template class num_get<char, istreambuf_iterator<char, char_traits<char> > >;

#  if !defined (_STLP_NO_WCHAR_T)
template class _STLP_CLASS_DECLSPEC  istreambuf_iterator<wchar_t, char_traits<wchar_t> >;
template class num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;

#  endif
#endif

_STLP_END_NAMESPACE




