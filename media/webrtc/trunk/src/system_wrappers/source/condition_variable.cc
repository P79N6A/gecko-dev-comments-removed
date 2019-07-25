









#if defined(_WIN32)
   #include <windows.h>
   #include "condition_variable_wrapper.h"
   #include "condition_variable_win.h"
#elif defined(WEBRTC_LINUX)
   #include <pthread.h>
   #include "condition_variable_wrapper.h"
   #include "condition_variable_posix.h"
#elif defined(WEBRTC_MAC) || defined(WEBRTC_MAC_INTEL)
   #include <pthread.h>
   #include "condition_variable_wrapper.h"
   #include "condition_variable_posix.h"
#endif

namespace webrtc {
ConditionVariableWrapper*
ConditionVariableWrapper::CreateConditionVariable()
{
#if defined(_WIN32)
    return new ConditionVariableWindows;
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC) || defined(WEBRTC_MAC_INTEL)
    return ConditionVariablePosix::Create();
#else
    return NULL;
#endif
}
} 
