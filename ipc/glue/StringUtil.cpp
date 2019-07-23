



































#include "base/string_util.h"





#include "base/sys_string_conversions.h"

#include "base/string_piece.h"
#include "base/string_util.h"

#include "build/build_config.h"



#ifdef WCHAR_T_IS_UTF16
#  define ICONV_WCHAR_T_ENCODING "UTF-16"
#else
#  define ICONV_WCHAR_T_ENCODING "WCHAR_T"
#endif





namespace base {
string16 SysWideToUTF16(const std::wstring& wide);


template<typename FromType, typename ToType>
ToType
GhettoStringConvert(const FromType& in)
{
  
  ToType out;
  out.resize(in.length());
  for (int i = 0; i < static_cast<int>(in.length()); ++i)
      out[i] = static_cast<typename ToType::value_type>(in[i]);
  return out;
}
}




std::string
WideToUTF8(const std::wstring& wide)
{
    return base::SysWideToUTF8(wide);
}

string16
UTF8ToUTF16(const std::string& utf8)
{
    
    return base::GhettoStringConvert<std::string, string16>(utf8);
}

std::wstring
UTF8ToWide(const StringPiece& utf8)
{
    return base::SysUTF8ToWide(utf8);
}

string16
WideToUTF16(const std::wstring& wide)
{
    return base::SysWideToUTF16(wide);
}

std::string
UTF16ToUTF8(const string16& utf16)
{
    
    return base::GhettoStringConvert<string16, std::string>(utf16);
}

namespace base {




#ifndef OS_MACOSX
std::string SysWideToUTF8(const std::wstring& wide) {
  
  return GhettoStringConvert<std::wstring, std::string>(wide);
}
#endif

string16 SysWideToUTF16(const std::wstring& wide)
{
#if defined(WCHAR_T_IS_UTF16)
  return wide;
#else
  
  return GhettoStringConvert<std::wstring, string16>(wide);
#endif
}

#ifndef OS_MACOSX
std::wstring SysUTF8ToWide(const StringPiece& utf8) {
  
  return GhettoStringConvert<StringPiece, std::wstring>(utf8);
}

std::string SysWideToNativeMB(const std::wstring& wide) {
  
  return SysWideToUTF8(wide);
}

std::wstring SysNativeMBToWide(const StringPiece& native_mb) {
  
  return SysUTF8ToWide(native_mb);
}
#endif

}  
