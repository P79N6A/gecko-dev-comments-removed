





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





int vswprintf(wchar_t* buffer, size_t size,
              const wchar_t* format, va_list arguments)
    WPRINTF_FORMAT(3, 0);





inline int snprintf(char* buffer, size_t size, const char* format, ...)
    PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}



inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...)
    WPRINTF_FORMAT(3, 4);
inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vswprintf(buffer, size, format, arguments);
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

}  

#if defined(OS_WIN)
#include "base/strings/string_util_win.h"
#elif defined(OS_POSIX)
#include "base/strings/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif










BASE_EXPORT const std::string& EmptyString();
BASE_EXPORT const std::wstring& EmptyWString();
BASE_EXPORT const string16& EmptyString16();

BASE_EXPORT extern const wchar_t kWhitespaceWide[];
BASE_EXPORT extern const char16 kWhitespaceUTF16[];
BASE_EXPORT extern const char kWhitespaceASCII[];

BASE_EXPORT extern const char kUtf8ByteOrderMark[];




BASE_EXPORT bool RemoveChars(const string16& input,
                             const char16 remove_chars[],
                             string16* output);
BASE_EXPORT bool RemoveChars(const std::string& input,
                             const char remove_chars[],
                             std::string* output);






BASE_EXPORT bool ReplaceChars(const string16& input,
                              const char16 replace_chars[],
                              const string16& replace_with,
                              string16* output);
BASE_EXPORT bool ReplaceChars(const std::string& input,
                              const char replace_chars[],
                              const std::string& replace_with,
                              std::string* output);




BASE_EXPORT bool TrimString(const std::wstring& input,
                            const wchar_t trim_chars[],
                            std::wstring* output);
BASE_EXPORT bool TrimString(const string16& input,
                            const char16 trim_chars[],
                            string16* output);
BASE_EXPORT bool TrimString(const std::string& input,
                            const char trim_chars[],
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
                                         string16* output);
BASE_EXPORT TrimPositions TrimWhitespaceASCII(const std::string& input,
                                              TrimPositions positions,
                                              std::string* output);



BASE_EXPORT TrimPositions TrimWhitespace(const std::string& input,
                                         TrimPositions positions,
                                         std::string* output);









BASE_EXPORT std::wstring CollapseWhitespace(
    const std::wstring& text,
    bool trim_sequences_with_line_breaks);
BASE_EXPORT string16 CollapseWhitespace(
    const string16& text,
    bool trim_sequences_with_line_breaks);
BASE_EXPORT std::string CollapseWhitespaceASCII(
    const std::string& text,
    bool trim_sequences_with_line_breaks);



BASE_EXPORT bool ContainsOnlyWhitespaceASCII(const std::string& str);
BASE_EXPORT bool ContainsOnlyWhitespace(const string16& str);



BASE_EXPORT bool ContainsOnlyChars(const std::wstring& input,
                                   const std::wstring& characters);
BASE_EXPORT bool ContainsOnlyChars(const string16& input,
                                   const string16& characters);
BASE_EXPORT bool ContainsOnlyChars(const std::string& input,
                                   const std::string& characters);



BASE_EXPORT std::string WideToASCII(const std::wstring& wide);
BASE_EXPORT std::string UTF16ToASCII(const string16& utf16);



BASE_EXPORT bool WideToLatin1(const std::wstring& wide, std::string* latin1);












BASE_EXPORT bool IsStringUTF8(const std::string& str);
BASE_EXPORT bool IsStringASCII(const std::wstring& str);
BASE_EXPORT bool IsStringASCII(const string16& str);



template <class str> inline void StringToLowerASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = base::ToLowerASCII(*i);
}

template <class str> inline str StringToLowerASCII(const str& s) {
  
  str output(s);
  StringToLowerASCII(&output);
  return output;
}



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
BASE_EXPORT bool LowerCaseEqualsASCII(const std::wstring& a, const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const string16& a, const char* b);


BASE_EXPORT bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                                      std::string::const_iterator a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                                      std::wstring::const_iterator a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(string16::const_iterator a_begin,
                                      string16::const_iterator a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const char* a_begin,
                                      const char* a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                                      const wchar_t* a_end,
                                      const char* b);
BASE_EXPORT bool LowerCaseEqualsASCII(const char16* a_begin,
                                      const char16* a_end,
                                      const char* b);



BASE_EXPORT bool EqualsASCII(const string16& a, const base::StringPiece& b);


BASE_EXPORT bool StartsWithASCII(const std::string& str,
                                 const std::string& search,
                                 bool case_sensitive);
BASE_EXPORT bool StartsWith(const std::wstring& str,
                            const std::wstring& search,
                            bool case_sensitive);
BASE_EXPORT bool StartsWith(const string16& str,
                            const string16& search,
                            bool case_sensitive);


BASE_EXPORT bool EndsWith(const std::string& str,
                          const std::string& search,
                          bool case_sensitive);
BASE_EXPORT bool EndsWith(const std::wstring& str,
                          const std::wstring& search,
                          bool case_sensitive);
BASE_EXPORT bool EndsWith(const string16& str,
                          const string16& search,
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
inline Char HexDigitToInt(Char c) {
  DCHECK(IsHexDigit(c));
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return 0;
}


inline bool IsWhitespace(wchar_t c) {
  return wcschr(kWhitespaceWide, c) != NULL;
}





BASE_EXPORT string16 FormatBytesUnlocalized(int64 bytes);



BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
    string16* str,
    string16::size_type start_offset,
    const string16& find_this,
    const string16& replace_with);
BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
    std::string* str,
    std::string::size_type start_offset,
    const std::string& find_this,
    const std::string& replace_with);







BASE_EXPORT void ReplaceSubstringsAfterOffset(
    string16* str,
    string16::size_type start_offset,
    const string16& find_this,
    const string16& replace_with);
BASE_EXPORT void ReplaceSubstringsAfterOffset(
    std::string* str,
    std::string::size_type start_offset,
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






BASE_EXPORT size_t Tokenize(const std::wstring& str,
                            const std::wstring& delimiters,
                            std::vector<std::wstring>* tokens);
BASE_EXPORT size_t Tokenize(const string16& str,
                            const string16& delimiters,
                            std::vector<string16>* tokens);
BASE_EXPORT size_t Tokenize(const std::string& str,
                            const std::string& delimiters,
                            std::vector<std::string>* tokens);
BASE_EXPORT size_t Tokenize(const base::StringPiece& str,
                            const base::StringPiece& delimiters,
                            std::vector<base::StringPiece>* tokens);


BASE_EXPORT string16 JoinString(const std::vector<string16>& parts, char16 s);
BASE_EXPORT std::string JoinString(
    const std::vector<std::string>& parts, char s);


BASE_EXPORT std::string JoinString(
    const std::vector<std::string>& parts,
    const std::string& separator);
BASE_EXPORT string16 JoinString(
    const std::vector<string16>& parts,
    const string16& separator);





BASE_EXPORT string16 ReplaceStringPlaceholders(
    const string16& format_string,
    const std::vector<string16>& subst,
    std::vector<size_t>* offsets);

BASE_EXPORT std::string ReplaceStringPlaceholders(
    const base::StringPiece& format_string,
    const std::vector<std::string>& subst,
    std::vector<size_t>* offsets);


BASE_EXPORT string16 ReplaceStringPlaceholders(const string16& format_string,
                                               const string16& a,
                                               size_t* offset);






BASE_EXPORT bool MatchPattern(const base::StringPiece& string,
                              const base::StringPiece& pattern);
BASE_EXPORT bool MatchPattern(const string16& string, const string16& pattern);




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
