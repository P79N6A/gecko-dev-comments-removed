



#ifndef SANDBOX_SRC_SANDBOX_UTILS_H_
#define SANDBOX_SRC_SANDBOX_UTILS_H_

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/nt_internals.h"

namespace sandbox {

void InitObjectAttribs(const base::string16& name,
                       ULONG attributes,
                       HANDLE root,
                       OBJECT_ATTRIBUTES* obj_attr,
                       UNICODE_STRING* uni_name);

}  

#endif  
