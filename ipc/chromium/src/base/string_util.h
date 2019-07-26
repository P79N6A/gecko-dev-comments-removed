





#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_

#include <stdarg.h>   
#include <ctype.h>

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










enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING
};
TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output);
TrimPositions TrimWhitespaceASCII(const std::string& input,
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
std::wstring UTF8ToWide(const ::StringPiece& utf8);

bool IsStringASCII(const std::wstring& str);
bool IsStringASCII(const std::string& str);
bool IsStringASCII(const string16& str);


std::string IntToString(int value);
std::wstring IntToWString(int value);
std::string UintToString(unsigned int value);
std::wstring UintToWString(unsigned int value);
std::string Int64ToString(int64_t value);
std::wstring Int64ToWString(int64_t value);
std::string Uint64ToString(uint64_t value);
std::wstring Uint64ToWString(uint64_t value);


std::string DoubleToString(double value);
std::wstring DoubleToWString(double value);











bool StringToInt(const std::string& input, int* output);
bool StringToInt(const string16& input, int* output);
bool StringToInt64(const std::string& input, int64_t* output);
bool StringToInt64(const string16& input, int64_t* output);




int StringToInt(const std::string& value);
int StringToInt(const string16& value);
int64_t StringToInt64(const std::string& value);
int64_t StringToInt64(const string16& value);


std::string StringPrintf(const char* format, ...);
std::wstring StringPrintf(const wchar_t* format, ...);


const std::string& SStringPrintf(std::string* dst, const char* format, ...);
const std::wstring& SStringPrintf(std::wstring* dst,
                                  const wchar_t* format, ...);


void StringAppendF(std::string* dst, const char* format, ...);
void StringAppendF(std::wstring* dst, const wchar_t* format, ...);








void SplitString(const std::wstring& str,
                 wchar_t s,
                 std::vector<std::wstring>* r);
void SplitString(const std::string& str,
                 char s,
                 std::vector<std::string>* r);

#endif  
