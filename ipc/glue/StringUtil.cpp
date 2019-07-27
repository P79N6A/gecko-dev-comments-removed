



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

std::wstring
UTF8ToWide(const StringPiece& utf8)
{
    return base::SysUTF8ToWide(utf8);
}

namespace base {





#if !defined(OS_MACOSX) && !defined(OS_WIN)
std::string SysWideToUTF8(const std::wstring& wide) {
  
  return GhettoStringConvert<std::wstring, std::string>(wide);
}
#endif

#if !defined(OS_MACOSX) && !defined(OS_WIN)
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
