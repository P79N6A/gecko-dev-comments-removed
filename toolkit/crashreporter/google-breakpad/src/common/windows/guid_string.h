






























#ifndef COMMON_WINDOWS_GUID_STRING_H__
#define COMMON_WINDOWS_GUID_STRING_H__

#include <guiddef.h>

#include <string>

namespace google_breakpad {

using std::wstring;

class GUIDString {
 public:
  
  
  static wstring GUIDToWString(GUID *guid);

  
  
  
  
  static wstring GUIDToSymbolServerWString(GUID *guid);
};

}  

#endif  
