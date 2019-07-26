



#ifndef BASE_SYS_STRING_CONVERSIONS_H_
#define BASE_SYS_STRING_CONVERSIONS_H_





#include <string>
#include "base/basictypes.h"
#include "base/string16.h"

class StringPiece;

namespace base {



std::string SysWideToUTF8(const std::wstring& wide);
std::wstring SysUTF8ToWide(const StringPiece& utf8);




std::string SysWideToNativeMB(const std::wstring& wide);
std::wstring SysNativeMBToWide(const StringPiece& native_mb);



#if defined(OS_WIN)




std::wstring SysMultiByteToWide(const StringPiece& mb, uint32_t code_page);
std::string SysWideToMultiByte(const std::wstring& wide, uint32_t code_page);

#endif  

}  

#endif  
