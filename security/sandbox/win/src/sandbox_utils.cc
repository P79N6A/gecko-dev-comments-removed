



#include "sandbox/win/src/sandbox_utils.h"

#include <windows.h>

#include "base/logging.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/internal_types.h"

namespace sandbox {

bool IsXPSP2OrLater() {
  base::win::Version version = base::win::GetVersion();
  return (version > base::win::VERSION_XP) ||
      ((version == base::win::VERSION_XP) &&
       (base::win::OSInfo::GetInstance()->service_pack().major >= 2));
}

void InitObjectAttribs(const std::wstring& name,
                       ULONG attributes,
                       HANDLE root,
                       OBJECT_ATTRIBUTES* obj_attr,
                       UNICODE_STRING* uni_name) {
  static RtlInitUnicodeStringFunction RtlInitUnicodeString;
  if (!RtlInitUnicodeString) {
    HMODULE ntdll = ::GetModuleHandle(kNtdllName);
    RtlInitUnicodeString = reinterpret_cast<RtlInitUnicodeStringFunction>(
      GetProcAddress(ntdll, "RtlInitUnicodeString"));
    DCHECK(RtlInitUnicodeString);
  }
  RtlInitUnicodeString(uni_name, name.c_str());
  InitializeObjectAttributes(obj_attr, uni_name, attributes, root, NULL);
}

};  
