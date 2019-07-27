





#include "AppleUtils.h"
#include "prlog.h"
#include "nsAutoPtr.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("AppleMedia");
  }
  return log;
}
#define WARN(...) PR_LOG(GetAppleMediaLog(), PR_LOG_WARNING, (__VA_ARGS__))
#else
#define WARN(...)
#endif

#define PROPERTY_ID_FORMAT "%c%c%c%c"
#define PROPERTY_ID_PRINT(x) ((x) >> 24), \
                             ((x) >> 16) & 0xff, \
                             ((x) >> 8) & 0xff, \
                              (x) & 0xff

namespace mozilla {

void
AppleUtils::SetCFDict(CFMutableDictionaryRef dict,
                      const char* key,
                      const char* value)
{
  
  AutoCFRelease<CFStringRef> keyRef =
    CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
  AutoCFRelease<CFStringRef> valueRef =
    CFStringCreateWithCString(NULL, value, kCFStringEncodingUTF8);
  CFDictionarySetValue(dict, keyRef, valueRef);
}

void
AppleUtils::SetCFDict(CFMutableDictionaryRef dict,
                      const char* key,
                      int32_t value)
{
  AutoCFRelease<CFNumberRef> valueRef =
    CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
  AutoCFRelease<CFStringRef> keyRef =
    CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
  CFDictionarySetValue(dict, keyRef, valueRef);
}

void
AppleUtils::SetCFDict(CFMutableDictionaryRef dict,
                      const char* key,
                      bool value)
{
  AutoCFRelease<CFStringRef> keyRef =
    CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
  CFDictionarySetValue(dict, keyRef, value ? kCFBooleanTrue : kCFBooleanFalse);
}

} 
