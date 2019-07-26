









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_POSIX_H_

#include "thread_wrapper.h"
#include <pthread.h>

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;

class ThreadPosix : public ThreadWrapper
{
public:
    static ThreadWrapper* Create(ThreadRunFunction func, ThreadObj obj,
                                 ThreadPriority prio, const char* threadName);

    ThreadPosix(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio,
                const char* threadName);
    ~ThreadPosix();

    
    virtual void SetNotAlive();
    virtual bool Start(unsigned int& id);
    
    virtual bool SetAffinity(const int* processorNumbers,
                             unsigned int amountOfProcessors);
    virtual bool Stop();
    virtual bool Shutdown();

    void Run();

private:
    int Construct();

private:
    
    ThreadRunFunction   _runFunction;
    ThreadObj           _obj;

    
    CriticalSectionWrapper* _crit_state;  
    bool                    _alive;
    bool                    _dead;
    ThreadPriority          _prio;
    EventWrapper*           _event;

    
    char                    _name[kThreadMaxNameLength];
    bool                    _setThreadName;

    
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID))
    pid_t                   _pid;
#endif
    pthread_attr_t          _attr;
    pthread_t               _thread;
};
} 

#endif 
