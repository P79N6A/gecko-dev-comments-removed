
















#include "stlport_prefix.h"

#include <locale>

_STLP_BEGIN_NAMESPACE



char   numpunct<char>::do_decimal_point() const {return '.';}
char   numpunct<char>::do_thousands_sep() const { return ','; }
string numpunct<char>::do_grouping()  const { return string();}
string numpunct<char>::do_truename()  const { return "true";}
string numpunct<char>::do_falsename() const { return "false"; }
numpunct<char>::~numpunct() {}

#if !defined (_STLP_NO_WCHAR_T)
wchar_t numpunct<wchar_t>::do_decimal_point() const { return L'.'; }
wchar_t numpunct<wchar_t>::do_thousands_sep() const { return L','; }
string numpunct<wchar_t>::do_grouping()   const { return string(); }
wstring numpunct<wchar_t>::do_truename()  const { return L"true"; }
wstring numpunct<wchar_t>::do_falsename() const { return L"false"; }
numpunct<wchar_t>::~numpunct() {}
#endif

_STLP_END_NAMESPACE




