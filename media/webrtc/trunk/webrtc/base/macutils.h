









#ifndef WEBRTC_BASE_MACUTILS_H__
#define WEBRTC_BASE_MACUTILS_H__

#include <CoreFoundation/CoreFoundation.h>
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <Carbon/Carbon.h>
#endif
#include <string>

namespace rtc {






bool ToUtf8(const CFStringRef str16, std::string* str8);
bool ToUtf16(const std::string& str8, CFStringRef* str16);

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
void DecodeFourChar(UInt32 fc, std::string* out);

enum MacOSVersionName {
  kMacOSUnknown,       
  kMacOSOlder,         
  kMacOSPanther,       
  kMacOSTiger,         
  kMacOSLeopard,       
  kMacOSSnowLeopard,   
  kMacOSLion,          
  kMacOSMountainLion,  
  kMacOSMavericks,     
  kMacOSNewer,         
};

bool GetOSVersion(int* major, int* minor, int* bugfix);
MacOSVersionName GetOSVersionName();
bool GetQuickTimeVersion(std::string* version);

#ifndef WEBRTC_MOZILLA_BUILD


bool RunAppleScript(const std::string& script);
#endif
#endif



}  

#endif  
