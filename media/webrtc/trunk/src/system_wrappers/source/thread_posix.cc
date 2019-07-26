











































#include "thread_posix.h"

#include <errno.h>
#include <string.h> 
#include <time.h>   
#include <unistd.h>
#ifdef WEBRTC_LINUX
#include <sys/types.h>
#include <sched.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
#endif

#if defined(WEBRTC_MAC)
#include <mach/mach.h>
#endif

#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/event_wrapper.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {
extern "C"
{
    static void* StartThread(void* lpParameter)
    {
        static_cast<ThreadPosix*>(lpParameter)->Run();
        return 0;
    }
}

ThreadWrapper* ThreadPosix::Create(ThreadRunFunction func, ThreadObj obj,
                                   ThreadPriority prio, const char* threadName)
{
    ThreadPosix* ptr = new ThreadPosix(func, obj, prio, threadName);
    if (!ptr)
    {
        return NULL;
    }
    const int error = ptr->Construct();
    if (error)
    {
        delete ptr;
        return NULL;
    }
    return ptr;
}

ThreadPosix::ThreadPosix(ThreadRunFunction func, ThreadObj obj,
                         ThreadPriority prio, const char* threadName)
    : _runFunction(func),
      _obj(obj),
      _crit_state(CriticalSectionWrapper::CreateCriticalSection()),
      _alive(false),
      _dead(true),
      _prio(prio),
      _event(EventWrapper::Create()),
      _name(),
      _setThreadName(false),
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID))
      _pid(-1),
#endif
      _attr(),
      _thread(0)
{
    if (threadName != NULL)
    {
        _setThreadName = true;
        strncpy(_name, threadName, kThreadMaxNameLength);
        _name[kThreadMaxNameLength - 1] = '\0';
    }
}

uint32_t ThreadWrapper::GetThreadId() {
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_LINUX)
  return static_cast<uint32_t>(syscall(__NR_gettid));
#elif defined(WEBRTC_MAC)
  return static_cast<uint32_t>(mach_thread_self());
#else
  return reinterpret_cast<uint32_t>(pthread_self());
#endif
}

int ThreadPosix::Construct()
{
    int result = 0;
#if !defined(WEBRTC_ANDROID) && !defined(WEBRTC_GONK)
    
    result = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (result != 0)
    {
        return -1;
    }
    result = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (result != 0)
    {
        return -1;
    }
#endif
    result = pthread_attr_init(&_attr);
    if (result != 0)
    {
        return -1;
    }
    return 0;
}

ThreadPosix::~ThreadPosix()
{
    pthread_attr_destroy(&_attr);
    delete _event;
    delete _crit_state;
}

#define HAS_THREAD_ID !defined(MAC_IPHONE) && !defined(MAC_IPHONE_SIM)  &&  \
                      !defined(WEBRTC_MAC) && !defined(WEBRTC_MAC_INTEL) && \
                      !defined(MAC_DYLIB)  && !defined(MAC_INTEL_DYLIB)
#if HAS_THREAD_ID
bool ThreadPosix::Start(unsigned int& threadID)
#else
bool ThreadPosix::Start(unsigned int& )
#endif
{
    if (!_runFunction)
    {
        return false;
    }
    int result = pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_DETACHED);
    
    result |= pthread_attr_setstacksize(&_attr, 1024*1024);
#ifdef WEBRTC_THREAD_RR
    const int policy = SCHED_RR;
#else
    const int policy = SCHED_FIFO;
#endif
    _event->Reset();
    result |= pthread_create(&_thread, &_attr, &StartThread, this);
    if (result != 0)
    {
        return false;
    }

    
    
    if (kEventSignaled != _event->Wait(WEBRTC_EVENT_10_SEC))
    {
        
        _runFunction = NULL;
        return false;
    }

#if HAS_THREAD_ID
    threadID = static_cast<unsigned int>(_thread);
#endif
    sched_param param;

    const int minPrio = sched_get_priority_min(policy);
    const int maxPrio = sched_get_priority_max(policy);
    if ((minPrio == EINVAL) || (maxPrio == EINVAL))
    {
        return false;
    }

    switch (_prio)
    {
    case kLowPriority:
        param.sched_priority = minPrio + 1;
        break;
    case kNormalPriority:
        param.sched_priority = (minPrio + maxPrio) / 2;
        break;
    case kHighPriority:
        param.sched_priority = maxPrio - 3;
        break;
    case kHighestPriority:
        param.sched_priority = maxPrio - 2;
        break;
    case kRealtimePriority:
        param.sched_priority = maxPrio - 1;
        break;
    }
    result = pthread_setschedparam(_thread, policy, &param);
    if (result == EINVAL)
    {
        return false;
    }
    return true;
}



#if (defined(WEBRTC_LINUX) && (!defined(WEBRTC_ANDROID)))
bool ThreadPosix::SetAffinity(const int* processorNumbers,
                              const unsigned int amountOfProcessors) {
  if (!processorNumbers || (amountOfProcessors == 0)) {
    return false;
  }
  cpu_set_t mask;
  CPU_ZERO(&mask);

  for (unsigned int processor = 0;
      processor < amountOfProcessors;
      processor++) {
    CPU_SET(processorNumbers[processor], &mask);
  }
#if defined(WEBRTC_ANDROID)
  
  const int result = syscall(__NR_sched_setaffinity,
                             _pid,
                             sizeof(mask),
                             &mask);
#else
  
  const int result = sched_setaffinity(_pid,
                                       sizeof(mask),
                                       &mask);
#endif
  if (result != 0) {
    return false;
  }
  return true;
}

#else



bool ThreadPosix::SetAffinity(const int* , const unsigned int)
{
    return false;
}
#endif

void ThreadPosix::SetNotAlive()
{
    CriticalSectionScoped cs(_crit_state);
    _alive = false;
}

bool ThreadPosix::Shutdown()
{
#if !defined(WEBRTC_ANDROID) && !defined(WEBRTC_GONK)
    if (_thread && (0 != pthread_cancel(_thread)))
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool ThreadPosix::Stop()
{
    bool dead = false;
    {
        CriticalSectionScoped cs(_crit_state);
        _alive = false;
        dead = _dead;
    }

    
    
    for (int i = 0; i < 1000 && !dead; i++)
    {
        timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 10*1000*1000;
        nanosleep(&t, NULL);
        {
            CriticalSectionScoped cs(_crit_state);
            dead = _dead;
        }
    }
    if (dead)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ThreadPosix::Run()
{
    {
        CriticalSectionScoped cs(_crit_state);
        _alive = true;
        _dead  = false;
    }
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID))
    _pid = GetThreadId();
#endif
    
    _event->Set();

    if (_setThreadName)
    {
#ifdef WEBRTC_LINUX
        prctl(PR_SET_NAME, (unsigned long)_name, 0, 0, 0);
#endif
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,-1,
                     "Thread with name:%s started ", _name);
    } else
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                     "Thread without name started");
    }
    bool alive = true;
    do
    {
        if (_runFunction)
        {
            if (!_runFunction(_obj))
            {
                alive = false;
            }
        }
        else
        {
            alive = false;
        }
        {
            CriticalSectionScoped cs(_crit_state);
            if (!alive) {
              _alive = false;
            }
            alive = _alive;
        }
    }
    while (alive);

    if (_setThreadName)
    {
        
        
        
        if (strcmp(_name, "Trace"))
        {
            WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,-1,
                         "Thread with name:%s stopped", _name);
        }
    }
    else
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,-1,
                     "Thread without name stopped");
    }
    {
        CriticalSectionScoped cs(_crit_state);
        _dead = true;
    }
}
} 
