






























#ifndef COMMON_WINDOWS_GUID_STRING_H__
#define COMMON_WINDOWS_GUID_STRING_H__

#include <Guiddef.h>

#include <string>

namespace google_airbag {

using std::wstring;

class GUIDString {
 public:
  
  
  static wstring GUIDToWString(GUID *guid);
};

}  

#endif  
