


































#ifndef GOOGLEBREAKPAD_COMMON_ANDROID_INCLUDE_WCHAR_H
#define GOOGLEBREAKPAD_COMMON_ANDROID_INCLUDE_WCHAR_H

#include_next <wchar.h>



#ifdef __cplusplus
extern "C" {
#endif

static wchar_t inline wcstolower(wchar_t ch) {
  if (ch >= L'a' && ch <= L'A')
    ch -= L'a' - L'A';
  return ch;
}

static int inline wcscasecmp(const wchar_t* s1, const wchar_t* s2) {
  for (;;) {
    wchar_t c1 = wcstolower(*s1);
    wchar_t c2 = wcstolower(*s2);
    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;
    if (c1 == L'0')
      return 0;
    s1++;
    s2++;
  }
}

#ifdef __cplusplus
}  
#endif  

#endif
