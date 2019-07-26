



#ifndef SANDBOX_SRC_SANDBOX_UTILS_H__
#define SANDBOX_SRC_SANDBOX_UTILS_H__

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"

namespace sandbox {


bool IsXPSP2OrLater();

void InitObjectAttribs(const std::wstring& name,
                       ULONG attributes,
                       HANDLE root,
                       OBJECT_ATTRIBUTES* obj_attr,
                       UNICODE_STRING* uni_name);

};  

#endif  
