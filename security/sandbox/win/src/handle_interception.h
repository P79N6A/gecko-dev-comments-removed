



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_HANDLE_INTERCEPTION_H_
#define SANDBOX_SRC_HANDLE_INTERCEPTION_H_

namespace sandbox {



ResultCode DuplicateHandleProxy(HANDLE source_handle,
                                DWORD target_process_id,
                                HANDLE* target_handle,
                                DWORD desired_access,
                                DWORD options);

}  

#endif  

