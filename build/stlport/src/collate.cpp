
















#include "stlport_prefix.h"

#include <locale>

_STLP_BEGIN_NAMESPACE



collate<char>::~collate() {}

int collate<char>::do_compare(const char* low1, const char* high1,
                              const char* low2, const char* high2) const
{ return _STLP_PRIV __lexicographical_compare_3way(low1, high1, low2, high2); }

string collate<char>::do_transform(const char* low, const char* high) const
{ return string(low, high); }

long collate<char>::do_hash(const char* low, const char* high) const {
  unsigned long result = 0;
  for ( ; low < high; ++low)
    result = 5 * result + *low;
  return result;
}

#if !defined (_STLP_NO_WCHAR_T)


collate<wchar_t>::~collate() {}

int
collate<wchar_t>::do_compare(const wchar_t* low1, const wchar_t* high1,
                             const wchar_t* low2, const wchar_t* high2) const
{ return _STLP_PRIV __lexicographical_compare_3way(low1, high1, low2, high2); }

wstring collate<wchar_t>::do_transform(const wchar_t* low, const wchar_t* high) const
{ return wstring(low, high); }

long collate<wchar_t>::do_hash(const wchar_t* low, const wchar_t* high) const {
  unsigned long result = 0;
  for ( ; low < high; ++low)
    result = 5 * result + *low;
  return result;
}
#endif

_STLP_END_NAMESPACE






