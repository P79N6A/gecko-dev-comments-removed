



#include <windows.h>

#include "sandbox/win/src/sandbox_nt_types.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {


SANDBOX_INTERCEPT HANDLE g_shared_section = NULL;


SANDBOX_INTERCEPT NtExports g_nt = {};

}  
