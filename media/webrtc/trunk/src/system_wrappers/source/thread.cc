









#include "thread_wrapper.h"

#if defined(_WIN32)
    #include "thread_win.h"
#else
    #include "thread_posix.h"
#endif

namespace webrtc {
ThreadWrapper* ThreadWrapper::CreateThread(ThreadRunFunction func,
                                           ThreadObj obj, ThreadPriority prio,
                                           const char* threadName)
{
#if defined(_WIN32)
    return new ThreadWindows(func, obj, prio, threadName);
#else
    return ThreadPosix::Create(func, obj, prio, threadName);
#endif
}
} 
