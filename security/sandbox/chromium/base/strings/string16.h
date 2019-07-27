



#ifndef BASE_STRINGS_STRING16_H_
#define BASE_STRINGS_STRING16_H_






















#include <stdio.h>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"

#if defined(WCHAR_T_IS_UTF16)

namespace base {

typedef wchar_t char16;
typedef std::wstring string16;
typedef std::char_traits<wchar_t> string16_char_traits;

}  

#elif defined(WCHAR_T_IS_UTF32)

namespace base {

typedef uint16 char16;




BASE_EXPORT int c16memcmp(const char16* s1, const char16* s2, size_t n);
BASE_EXPORT size_t c16len(const char16* s);
BASE_EXPORT const char16* c16memchr(const char16* s, char16 c, size_t n);
BASE_EXPORT char16* c16memmove(char16* s1, const char16* s2, size_t n);
BASE_EXPORT char16* c16memcpy(char16* s1, const char16* s2, size_t n);
BASE_EXPORT char16* c16memset(char16* s, char16 c, size_t n);

struct string16_char_traits {
  typedef char16 char_type;
  typedef int int_type;

  
  
  COMPILE_ASSERT(sizeof(int_type) > sizeof(char_type), unexpected_type_width);

  typedef std::streamoff off_type;
  typedef mbstate_t state_type;
  typedef std::fpos<state_type> pos_type;

  static void assign(char_type& c1, const char_type& c2) {
    c1 = c2;
  }

  static bool eq(const char_type& c1, const char_type& c2) {
    return c1 == c2;
  }
  static bool lt(const char_type& c1, const char_type& c2) {
    return c1 < c2;
  }

  static int compare(const char_type* s1, const char_type* s2, size_t n) {
    return c16memcmp(s1, s2, n);
  }

  static size_t length(const char_type* s) {
    return c16len(s);
  }

  static const char_type* find(const char_type* s, size_t n,
                               const char_type& a) {
    return c16memchr(s, a, n);
  }

  static char_type* move(char_type* s1, const char_type* s2, int_type n) {
    return c16memmove(s1, s2, n);
  }

  static char_type* copy(char_type* s1, const char_type* s2, size_t n) {
    return c16memcpy(s1, s2, n);
  }

  static char_type* assign(char_type* s, size_t n, char_type a) {
    return c16memset(s, a, n);
  }

  static int_type not_eof(const int_type& c) {
    return eq_int_type(c, eof()) ? 0 : c;
  }

  static char_type to_char_type(const int_type& c) {
    return char_type(c);
  }

  static int_type to_int_type(const char_type& c) {
    return int_type(c);
  }

  static bool eq_int_type(const int_type& c1, const int_type& c2) {
    return c1 == c2;
  }

  static int_type eof() {
    return static_cast<int_type>(EOF);
  }
};

typedef std::basic_string<char16, base::string16_char_traits> string16;

BASE_EXPORT extern std::ostream& operator<<(std::ostream& out,
                                            const string16& str);


BASE_EXPORT extern void PrintTo(const string16& str, std::ostream* out);

}  








































extern template
class BASE_EXPORT std::basic_string<base::char16, base::string16_char_traits>;

#endif  

#endif  
