





#ifndef BASE_STRINGS_STRING_UTIL_H_
#define BASE_STRINGS_STRING_UTIL_H_

#include <ctype.h>
#include <stdarg.h>   

#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"  

namespace base {









int strcasecmp(const char* s1, const char* s2);




int strncasecmp(const char* s1, const char* s2, size_t count);


int strncmp16(const char16* s1, const char16* s2, size_t count);




int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments)
    PRINTF_FORMAT(3, 0);





inline int snprintf(char* buffer, size_t size, const char* format, ...)
    PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}







BASE_EXPORT size_t strlcpy(char* dst, const char* src, size_t dst_size);
BASE_EXPORT size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);






















BASE_EXPORT bool IsWprintfFormatPortable(const wchar_t* format);



template <class Char> inline Char ToLowerASCII(Char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}



template <class Char> inline Char ToUpperASCII(Char c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}



template<typename Char> struct CaseInsensitiveCompare {
 public:
  bool operator()(Char x, Char y) const {
    
    
    return tolower(x) == tolower(y);
  }
};

template<typename Char> struct CaseInsensitiveCompareASCII {
 public:
  bool operator()(Char x, Char y) const {
    return ToLowerASCII(x) == ToLowerASCII(y);
  }
};














BASE_EXPORT const std::string& EmptyString();
BASE_EXPORT const string16& EmptyString16();



BASE_EXPORT extern const wchar_t kWhitespaceWide[];
BASE_EXPORT extern const char16 kWhitespaceUTF16[];
BASE_EXPORT extern const char kWhitespaceASCII[];


BASE_EXPORT extern const char kUtf8ByteOrderMark[];




BASE_EXPORT bool RemoveChars(const string16& input,
                             const base::StringPiece16& remove_chars,
                             string16* output);
BASE_EXPORT bool RemoveChars(const std::string& input,
                             const base::StringPiece& remove_chars,
                             std::string* output);






BASE_EXPORT bool ReplaceChars(const string16& input,
                              const base::StringPiece16& replace_chars,
                              const string16& replace_with,
                              string16* output);
BASE_EXPORT bool ReplaceChars(const std::string& input,
                              const base::StringPiece& replace_chars,
                              const std::string& replace_with,
                              std::string* output);




BASE_EXPORT bool TrimString(const string16& input,
                            const base::StringPiece16& trim_chars,
                            string16* output);
BASE_EXPORT bool TrimString(const std::string& input,
                            const base::StringPiece& trim_chars,
                            std::string* output);



BASE_EXPORT void TruncateUTF8ToByteSize(const std::string& input,
                                        const size_t byte_size,
                                        std::string* output);








enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};
BASE_EXPORT TrimPositions TrimWhitespace(const string16& input,
                                         TrimPositions positions,
                                         base::string16* output);
BASE_EXPORT TrimPositions TrimWhitespaceASCII(const std::string& input,
                                              TrimPositions positions,
                                              std::string* output);



BASE_EXPORT TrimPositions TrimWhitespace(const std::string& input,
                                         TrimPositions positions,
                                         std::string* output);









BASE_EXPORT string16 CollapseWhitespace(
    const string16& text,
    bool trim_sequences_with_line_breaks);
BASE_EXPORT std::string CollapseWhitespaceASCII(
    const std::string& text,
    bool trim_sequences_with_line_breaks);



BASE_EXPORT bool ContainsOnlyChars(const StringPiece& input,
                                   const StringPiece& characters);
BASE_EXPORT bool ContainsOnlyChars(const StringPiece16& input,
                                   const StringPiece16& characters);















BASE_EXPORT bool IsStringUTF8(const std::string& str);
BASE_EXPORT bool IsStringASCII(const StringPiece& str);
BASE_EXPORT bool IsStringASCII(const StringPiece16& str);


BASE_EXPORT bool IsStringASCII(const string16& str);
#if defined(WCHAR_T_IS_UTF32)
BASE_EXPORT bool IsStringASCII(const std::wstring& str);
#endif



template <class str> inline void StringToLowerASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = ToLowerASCII(*i);
}

template <class str> inline str StringToLowerASCII(const str& s) {
  
  str output(s);
  StringToLowerASCII(&output);
  return output;
}

}  

#if defined(OS_WIN)
#include "base/strings/string_util_win.h"
#elif defined(OS_POSIX)
#include "base/strings/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif



template <class str> inline void StringToUpperASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = base::ToUpperASCII(*i);
}

template <class str> inline str StringToUpperASCII(const str& s) {
  
  str output(s);
  StringToUpperASCII(&output);
  return output;
}





BASE_EXPORT bool LowerCaseEqualsASCII(const std::string& a, const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const base::string16& a, const char* b);


BASE_EXPORT bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                                      std::string::const_iterator a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(base::string16::const_iterator a_begin,
                                      base::string16::const_iterator a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const char* a_begin,
                                      const char* a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const base::char16* a_begin,
                                      const base::char16* a_end,
                                      const char* b);



BASE_EXPORT bool EqualsASCII(const base::string16& a, const base::StringPiece& b);


BASE_EXPORT bool StartsWithASCII(const std::string& str,
                                 const std::string& search,
                                 bool case_sensitive);
BASE_EXPORT bool StartsWith(const base::string16& str,
                            const base::string16& search,
                            bool case_sensitive);


BASE_EXPORT bool EndsWith(const std::string& str,
                          const std::string& search,
                          bool case_sensitive);
BASE_EXPORT bool EndsWith(const base::string16& str,
                          const base::string16& search,
                          bool case_sensitive);




template <typename Char>
inline bool IsAsciiWhitespace(Char c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
template <typename Char>
inline bool IsAsciiAlpha(Char c) {
  return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}
template <typename Char>
inline bool IsAsciiDigit(Char c) {
  return c >= '0' && c <= '9';
}

template <typename Char>
inline bool IsHexDigit(Char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}

template <typename Char>
inline char HexDigitToInt(Char c) {
  DCHECK(IsHexDigit(c));
  if (c >= '0' && c <= '9')
    return static_cast<char>(c - '0');
  if (c >= 'A' && c <= 'F')
    return static_cast<char>(c - 'A' + 10);
  if (c >= 'a' && c <= 'f')
    return static_cast<char>(c - 'a' + 10);
  return 0;
}


inline bool IsWhitespace(wchar_t c) {
  return wcschr(base::kWhitespaceWide, c) != NULL;
}





BASE_EXPORT base::string16 FormatBytesUnlocalized(int64 bytes);



BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
    base::string16* str,
    size_t start_offset,
    const base::string16& find_this,
    const base::string16& replace_with);
BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
    std::string* str,
    size_t start_offset,
    const std::string& find_this,
    const std::string& replace_with);







BASE_EXPORT void ReplaceSubstringsAfterOffset(
    base::string16* str,
    size_t start_offset,
    const base::string16& find_this,
    const base::string16& replace_with);
BASE_EXPORT void ReplaceSubstringsAfterOffset(std::string* str,
                                              size_t start_offset,
                                              const std::string& find_this,
                                              const std::string& replace_with);





















template <class string_type>
inline typename string_type::value_type* WriteInto(string_type* str,
                                                   size_t length_with_null) {
  DCHECK_GT(length_with_null, 1u);
  str->reserve(length_with_null);
  str->resize(length_with_null - 1);
  return &((*str)[0]);
}






BASE_EXPORT size_t Tokenize(const base::string16& str,
                            const base::string16& delimiters,
                            std::vector<base::string16>* tokens);
BASE_EXPORT size_t Tokenize(const std::string& str,
                            const std::string& delimiters,
                            std::vector<std::string>* tokens);
BASE_EXPORT size_t Tokenize(const base::StringPiece& str,
                            const base::StringPiece& delimiters,
                            std::vector<base::StringPiece>* tokens);


BASE_EXPORT base::string16 JoinString(const std::vector<base::string16>& parts,
                                      base::char16 s);
BASE_EXPORT std::string JoinString(
    const std::vector<std::string>& parts, char s);


BASE_EXPORT std::string JoinString(
    const std::vector<std::string>& parts,
    const std::string& separator);
BASE_EXPORT base::string16 JoinString(
    const std::vector<base::string16>& parts,
    const base::string16& separator);





BASE_EXPORT base::string16 ReplaceStringPlaceholders(
    const base::string16& format_string,
    const std::vector<base::string16>& subst,
    std::vector<size_t>* offsets);

BASE_EXPORT std::string ReplaceStringPlaceholders(
    const base::StringPiece& format_string,
    const std::vector<std::string>& subst,
    std::vector<size_t>* offsets);


BASE_EXPORT base::string16 ReplaceStringPlaceholders(
    const base::string16& format_string,
    const base::string16& a,
    size_t* offset);






BASE_EXPORT bool MatchPattern(const base::StringPiece& string,
                              const base::StringPiece& pattern);
BASE_EXPORT bool MatchPattern(const base::string16& string,
                              const base::string16& pattern);




template<typename T>
struct ToUnsigned {
  typedef T Unsigned;
};

template<>
struct ToUnsigned<char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<signed char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<wchar_t> {
#if defined(WCHAR_T_IS_UTF16)
  typedef unsigned short Unsigned;
#elif defined(WCHAR_T_IS_UTF32)
  typedef uint32 Unsigned;
#endif
};
template<>
struct ToUnsigned<short> {
  typedef unsigned short Unsigned;
};

#endif  
