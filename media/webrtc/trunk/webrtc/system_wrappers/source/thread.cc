









#include "webrtc/system_wrappers/interface/thread_wrapper.h"

#if defined(_WIN32)
#include "webrtc/system_wrappers/source/thread_win.h"
#else
#include "webrtc/system_wrappers/source/thread_posix.h"
#endif

namespace webrtc {

ThreadWrapper* ThreadWrapper::CreateThread(ThreadRunFunction func,
                                           ThreadObj obj, ThreadPriority prio,
                                           const char* thread_name) {
#if defined(_WIN32)
  return new ThreadWindows(func, obj, prio, thread_name);
#else
  return ThreadPosix::Create(func, obj, prio, thread_name);
#endif
}

} 
