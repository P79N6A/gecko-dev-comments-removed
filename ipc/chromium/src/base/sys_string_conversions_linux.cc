



#include "base/sys_string_conversions.h"

#include "base/string_piece.h"
#include "base/string_util.h"

namespace base {

std::string SysWideToUTF8(const std::wstring& wide) {
  
  
  return WideToUTF8(wide);
}
std::wstring SysUTF8ToWide(const StringPiece& utf8) {
  
  
  std::wstring out;
  UTF8ToWide(utf8.data(), utf8.size(), &out);
  return out;
}

std::string SysWideToNativeMB(const std::wstring& wide) {
  
  return SysWideToUTF8(wide);
}

std::wstring SysNativeMBToWide(const StringPiece& native_mb) {
  
  return SysUTF8ToWide(native_mb);
}

}  
