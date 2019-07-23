



#ifndef BASE_SYS_STRING_CONVERSIONS_H_
#define BASE_SYS_STRING_CONVERSIONS_H_





#include <string>
#include "base/basictypes.h"
#include "base/string16.h"

#if defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#ifdef __OBJC__
@class NSString;
#else
class NSString;
#endif
#endif  

class StringPiece;

namespace base {



std::string SysWideToUTF8(const std::wstring& wide);
std::wstring SysUTF8ToWide(const StringPiece& utf8);




std::string SysWideToNativeMB(const std::wstring& wide);
std::wstring SysNativeMBToWide(const StringPiece& native_mb);



#if defined(OS_WIN)




std::wstring SysMultiByteToWide(const StringPiece& mb, uint32 code_page);
std::string SysWideToMultiByte(const std::wstring& wide, uint32 code_page);

#endif  



#if defined(OS_MACOSX)





CFStringRef SysUTF8ToCFStringRef(const std::string& utf8);
CFStringRef SysUTF16ToCFStringRef(const string16& utf16);
CFStringRef SysWideToCFStringRef(const std::wstring& wide);


NSString* SysUTF8ToNSString(const std::string& utf8);
NSString* SysUTF16ToNSString(const string16& utf16);
NSString* SysWideToNSString(const std::wstring& wide);


std::string SysCFStringRefToUTF8(CFStringRef ref);
string16 SysCFStringRefToUTF16(CFStringRef ref);
std::wstring SysCFStringRefToWide(CFStringRef ref);


std::string SysNSStringToUTF8(NSString* ref);
string16 SysNSStringToUTF16(NSString* ref);
std::wstring SysNSStringToWide(NSString* ref);

#endif  

}  

#endif  
