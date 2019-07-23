





#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_

#include <stdarg.h>   

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/string16.h"
#include "base/string_piece.h"  



namespace base {









int strcasecmp(const char* s1, const char* s2);




int strncasecmp(const char* s1, const char* s2, size_t count);




int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments);





int vswprintf(wchar_t* buffer, size_t size,
              const wchar_t* format, va_list arguments);



inline int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vswprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}







size_t strlcpy(char* dst, const char* src, size_t dst_size);
size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);






















bool IsWprintfFormatPortable(const wchar_t* format);

}  

#if defined(OS_WIN)
#include "base/string_util_win.h"
#elif defined(OS_POSIX)
#include "base/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif

#ifdef CHROMIUM_MOZILLA_BUILD
namespace base {
#endif




const std::string& EmptyString();
const std::wstring& EmptyWString();
const string16& EmptyString16();
#ifdef CHROMIUM_MOZILLA_BUILD
}
#endif

extern const wchar_t kWhitespaceWide[];
extern const char kWhitespaceASCII[];


extern const char* const kCodepageUTF8;



bool TrimString(const std::wstring& input,
                const wchar_t trim_chars[],
                std::wstring* output);
bool TrimString(const std::string& input,
                const char trim_chars[],
                std::string* output);










enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};
TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output);
TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output);
TrimPositions TrimWhitespaceUTF8(const std::string& input,
                                 TrimPositions positions,
                                 std::string* output);



TrimPositions TrimWhitespace(const std::string& input,
                             TrimPositions positions,
                             std::string* output);









std::wstring CollapseWhitespace(const std::wstring& text,
                                bool trim_sequences_with_line_breaks);


std::string WideToASCII(const std::wstring& wide);
std::wstring ASCIIToWide(const std::string& ascii);
std::string UTF16ToASCII(const string16& utf16);
string16 ASCIIToUTF16(const std::string& ascii);







bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output);
std::string WideToUTF8(const std::wstring& wide);
bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output);
std::wstring UTF8ToWide(const StringPiece& utf8);

bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output);
string16 WideToUTF16(const std::wstring& wide);
bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output);
std::wstring UTF16ToWide(const string16& utf16);

bool UTF8ToUTF16(const char* src, size_t src_len, string16* output);
string16 UTF8ToUTF16(const std::string& utf8);
bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output);
std::string UTF16ToUTF8(const string16& utf16);






#if defined(OS_WIN)
# define WideToUTF16Hack
# define UTF16ToWideHack
#else
# define WideToUTF16Hack WideToUTF16
# define UTF16ToWideHack UTF16ToWide
#endif


class OnStringUtilConversionError {
 public:
  enum Type {
    
    FAIL,

    
    
    SKIP,
  };

 private:
  OnStringUtilConversionError();
};




bool WideToCodepage(const std::wstring& wide,
                    const char* codepage_name,
                    OnStringUtilConversionError::Type on_error,
                    std::string* encoded);
bool CodepageToWide(const std::string& encoded,
                    const char* codepage_name,
                    OnStringUtilConversionError::Type on_error,
                    std::wstring* wide);



bool WideToLatin1(const std::wstring& wide, std::string* latin1);





bool IsString8Bit(const std::wstring& str);
bool IsStringUTF8(const std::string& str);
bool IsStringWideUTF8(const std::wstring& str);
bool IsStringASCII(const std::wstring& str);
bool IsStringASCII(const std::string& str);
bool IsStringASCII(const string16& str);



template <class Char> inline Char ToLowerASCII(Char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}



template <class str> inline void StringToLowerASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = ToLowerASCII(*i);
}

template <class str> inline str StringToLowerASCII(const str& s) {
  
  str output(s);
  StringToLowerASCII(&output);
  return output;
}



template <class Char> inline Char ToUpperASCII(Char c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}



template <class str> inline void StringToUpperASCII(str* s) {
  for (typename str::iterator i = s->begin(); i != s->end(); ++i)
    *i = ToUpperASCII(*i);
}

template <class str> inline str StringToUpperASCII(const str& s) {
  
  str output(s);
  StringToUpperASCII(&output);
  return output;
}





bool LowerCaseEqualsASCII(const std::string& a, const char* b);
bool LowerCaseEqualsASCII(const std::wstring& a, const char* b);


bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end,
                          const char* b);
bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                          const wchar_t* a_end,
                          const char* b);


bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive);
bool StartsWith(const std::wstring& str,
                const std::wstring& search,
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


inline bool IsWhitespace(wchar_t c) {
  return wcschr(kWhitespaceWide, c) != NULL;
}



enum DataUnits {
  DATA_UNITS_BYTE = 0,
  DATA_UNITS_KILOBYTE,
  DATA_UNITS_MEGABYTE,
  DATA_UNITS_GIGABYTE,
};



DataUnits GetByteDisplayUnits(int64 bytes);





std::wstring FormatBytes(int64 bytes, DataUnits units, bool show_units);




std::wstring FormatSpeed(int64 bytes, DataUnits units, bool show_units);



std::wstring FormatNumber(int64 number);



void ReplaceFirstSubstringAfterOffset(string16* str,
                                      string16::size_type start_offset,
                                      const string16& find_this,
                                      const string16& replace_with);
void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      std::string::size_type start_offset,
                                      const std::string& find_this,
                                      const std::string& replace_with);







void ReplaceSubstringsAfterOffset(string16* str,
                                  string16::size_type start_offset,
                                  const string16& find_this,
                                  const string16& replace_with);
void ReplaceSubstringsAfterOffset(std::string* str,
                                  std::string::size_type start_offset,
                                  const std::string& find_this,
                                  const std::string& replace_with);


std::string IntToString(int value);
std::wstring IntToWString(int value);
std::string UintToString(unsigned int value);
std::wstring UintToWString(unsigned int value);
std::string Int64ToString(int64 value);
std::wstring Int64ToWString(int64 value);
std::string Uint64ToString(uint64 value);
std::wstring Uint64ToWString(uint64 value);


std::string DoubleToString(double value);
std::wstring DoubleToWString(double value);











bool StringToInt(const std::string& input, int* output);
bool StringToInt(const string16& input, int* output);
bool StringToInt64(const std::string& input, int64* output);
bool StringToInt64(const string16& input, int64* output);
bool HexStringToInt(const std::string& input, int* output);
bool HexStringToInt(const string16& input, int* output);





bool HexStringToBytes(const std::string& input, std::vector<uint8>* output);
bool HexStringToBytes(const string16& input, std::vector<uint8>* output);







bool StringToDouble(const std::string& input, double* output);
bool StringToDouble(const string16& input, double* output);




int StringToInt(const std::string& value);
int StringToInt(const string16& value);
int64 StringToInt64(const std::string& value);
int64 StringToInt64(const string16& value);
int HexStringToInt(const std::string& value);
int HexStringToInt(const string16& value);
double StringToDouble(const std::string& value);
double StringToDouble(const string16& value);


std::string StringPrintf(const char* format, ...);
std::wstring StringPrintf(const wchar_t* format, ...);


const std::string& SStringPrintf(std::string* dst, const char* format, ...);
const std::wstring& SStringPrintf(std::wstring* dst,
                                  const wchar_t* format, ...);


void StringAppendF(std::string* dst, const char* format, ...);
void StringAppendF(std::wstring* dst, const wchar_t* format, ...);



void StringAppendV(std::string* dst, const char* format, va_list ap);
void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap);
















template <class string_type>
inline typename string_type::value_type* WriteInto(string_type* str,
                                                   size_t length_with_null) {
  str->reserve(length_with_null);
  str->resize(length_with_null - 1);
  return &((*str)[0]);
}





#if defined(CHROMIUM_MOZILLA_BUILD)
template<typename Char> struct chromium_CaseInsensitiveCompare {
#else
template<typename Char> struct CaseInsensitiveCompare {
#endif
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








void SplitString(const std::wstring& str,
                 wchar_t s,
                 std::vector<std::wstring>* r);
void SplitString(const std::string& str,
                 char s,
                 std::vector<std::string>* r);


void SplitStringDontTrim(const std::wstring& str,
                         wchar_t s,
                         std::vector<std::wstring>* r);
void SplitStringDontTrim(const std::string& str,
                         char s,
                         std::vector<std::string>* r);


std::wstring JoinString(const std::vector<std::wstring>& parts, wchar_t s);
std::string JoinString(const std::vector<std::string>& parts, char s);









void SplitStringAlongWhitespace(const std::wstring& str,
                                std::vector<std::wstring>* result);




string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   size_t* offset);

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   const string16& b,
                                   std::vector<size_t>* offsets);

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   const string16& b,
                                   const string16& c,
                                   std::vector<size_t>* offsets);

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   const string16& b,
                                   const string16& c,
                                   const string16& d,
                                   std::vector<size_t>* offsets);







bool ElideString(const std::wstring& input, int max_len, std::wstring* output);






bool MatchPattern(const std::wstring& string, const std::wstring& pattern);
bool MatchPattern(const std::string& string, const std::string& pattern);








std::string HexEncode(const void* bytes, size_t size);


#endif
