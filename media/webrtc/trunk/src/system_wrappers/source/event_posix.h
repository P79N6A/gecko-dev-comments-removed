









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_POSIX_H_

#include "event_wrapper.h"

#include <pthread.h>
#include <time.h>

#include "thread_wrapper.h"

namespace webrtc {
enum State
{
    kUp = 1,
    kDown = 2
};

class EventPosix : public EventWrapper
{
public:
    static EventWrapper* Create();

    virtual ~EventPosix();

    virtual EventTypeWrapper Wait(unsigned long maxTime);
    virtual bool Set();
    virtual bool Reset();

    virtual bool StartTimer(bool periodic, unsigned long time);
    virtual bool StopTimer();

private:
    EventPosix();
    int Construct();

    static bool Run(ThreadObj obj);
    bool Process();
    EventTypeWrapper Wait(timespec& tPulse);


private:
    pthread_cond_t  cond;
    pthread_mutex_t mutex;

    ThreadWrapper* _timerThread;
    EventPosix*    _timerEvent;
    timespec       _tCreate;

    bool          _periodic;
    unsigned long _time;  
    unsigned long _count;
    State         _state;
};
} 

#endif 
