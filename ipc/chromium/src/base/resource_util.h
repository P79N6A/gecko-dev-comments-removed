






#ifndef BASE_RESOURCE_UTIL_H__
#define BASE_RESOURCE_UTIL_H__

#include <windows.h>
#include <string>

#include "base/basictypes.h"

namespace base {



bool GetDataResourceFromModule(HMODULE module, int resource_id,
                               void** data, size_t* length);
}  

#endif 
