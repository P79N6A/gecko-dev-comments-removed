



#include "base/string16.h"

#if defined(WCHAR_T_IS_UTF16)

#error This file should not be used on 2-byte wchar_t systems




#elif defined(WCHAR_T_IS_UTF32)

#include "base/string_util.h"

namespace base {

int c16memcmp(const char16* s1, const char16* s2, size_t n) {
  
  while (n-- > 0) {
    if (*s1 != *s2) {
      
      return ((*s1 < *s2) ? -1 : 1);
    }
    ++s1;
    ++s2;
  }
  return 0;
}

size_t c16len(const char16* s) {
  const char16 *s_orig = s;
  while (*s) {
    ++s;
  }
  return s - s_orig;
}

const char16* c16memchr(const char16* s, char16 c, size_t n) {
  while (n-- > 0) {
    if (*s == c) {
      return s;
    }
    ++s;
  }
  return 0;
}

char16* c16memmove(char16* s1, const char16* s2, size_t n) {
  return reinterpret_cast<char16*>(memmove(s1, s2, n * sizeof(char16)));
}

char16* c16memcpy(char16* s1, const char16* s2, size_t n) {
  return reinterpret_cast<char16*>(memcpy(s1, s2, n * sizeof(char16)));
}

char16* c16memset(char16* s, char16 c, size_t n) {
  char16 *s_orig = s;
  while (n-- > 0) {
    *s = c;
    ++s;
  }
  return s_orig;
}

}  

template class std::basic_string<char16, base::string16_char_traits>;

#endif  
