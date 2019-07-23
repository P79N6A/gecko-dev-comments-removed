
































#include <wchar.h>

#include "common/windows/string_utils-inl.h"

#include "common/windows/guid_string.h"

namespace google_airbag {


wstring GUIDString::GUIDToWString(GUID *guid) {
  wchar_t guid_string[37];
  WindowsStringUtils::safe_swprintf(
      guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
      L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      guid->Data1, guid->Data2, guid->Data3,
      guid->Data4[0], guid->Data4[1], guid->Data4[2],
      guid->Data4[3], guid->Data4[4], guid->Data4[5],
      guid->Data4[6], guid->Data4[7]);
  return wstring(guid_string);
}

}  
