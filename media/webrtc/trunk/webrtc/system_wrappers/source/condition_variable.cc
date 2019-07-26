









#if defined(_WIN32)
#include <windows.h>
#include "condition_variable_win.h"
#include "condition_variable_wrapper.h"
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
#include <pthread.h>
#include "condition_variable_posix.h"
#include "condition_variable_wrapper.h"
#endif

namespace webrtc {

ConditionVariableWrapper* ConditionVariableWrapper::CreateConditionVariable() {
#if defined(_WIN32)
  return new ConditionVariableWindows;
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
  return ConditionVariablePosix::Create();
#else
  return NULL;
#endif
}

} 
