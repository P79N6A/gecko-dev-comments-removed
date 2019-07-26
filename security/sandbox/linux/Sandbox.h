





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "base/process_util.h"

namespace mozilla {

void SetCurrentProcessSandbox(base::ChildPrivileges aPrivs);

} 

#endif 

