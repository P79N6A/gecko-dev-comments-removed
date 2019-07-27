







































#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_STRING_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_STRING_H_

#ifdef __BORLANDC__

# include <mem.h>
#endif

#include <string.h>
#include <string>

#include "gtest/internal/gtest-port.h"

namespace testing {
namespace internal {


class GTEST_API_ String {
 public:
  

  
  
  
  
  
  
  
  static const char* CloneCString(const char* c_str);

#if GTEST_OS_WINDOWS_MOBILE
  
  
  

  
  
  
  
  
  
  
  
  static LPCWSTR AnsiToUtf16(const char* c_str);

  
  
  
  
  
  
  
  
  static const char* Utf16ToAnsi(LPCWSTR utf16_str);
#endif

  
  
  
  
  
  static bool CStringEquals(const char* lhs, const char* rhs);

  
  
  
  
  static std::string ShowWideCString(const wchar_t* wide_c_str);

  
  
  
  
  
  
  static bool WideCStringEquals(const wchar_t* lhs, const wchar_t* rhs);

  
  
  
  
  
  
  static bool CaseInsensitiveCStringEquals(const char* lhs,
                                           const char* rhs);

  
  
  
  
  
  
  
  
  
  
  
  
  static bool CaseInsensitiveWideCStringEquals(const wchar_t* lhs,
                                               const wchar_t* rhs);

  
  
  static bool EndsWithCaseInsensitive(
      const std::string& str, const std::string& suffix);

  
  static std::string FormatIntWidth2(int value);  

  
  static std::string FormatHexInt(int value);

  
  static std::string FormatByte(unsigned char value);

 private:
  String();  
};  



GTEST_API_ std::string StringStreamToString(::std::stringstream* stream);

}  
}  

#endif  
