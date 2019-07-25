









#include "event_win.h"

#include "Mmsystem.h"

namespace webrtc {
EventWindows::EventWindows()
    : _event(::CreateEvent(NULL  ,
                           FALSE ,
                           FALSE ,
                           NULL  )),
      _timerID(NULL)
{
}

EventWindows::~EventWindows()
{
    CloseHandle(_event);
}

bool EventWindows::Set()
{
    
    return SetEvent(_event) == 1 ? true : false;
}

bool EventWindows::Reset()
{
    return ResetEvent(_event) == 1 ? true : false;
}

EventTypeWrapper EventWindows::Wait(unsigned long maxTime)
{
    unsigned long res = WaitForSingleObject(_event, maxTime);
    switch(res)
    {
    case WAIT_OBJECT_0:
        return kEventSignaled;
    case WAIT_TIMEOUT:
        return kEventTimeout;
    default:
        return kEventError;
    }
}

bool EventWindows::StartTimer(bool periodic, unsigned long time)
{
    if (_timerID != NULL)
    {
        timeKillEvent(_timerID);
        _timerID=NULL;
    }
    if (periodic)
    {
        _timerID=timeSetEvent(time, 0,(LPTIMECALLBACK)HANDLE(_event),0,
                              TIME_PERIODIC|TIME_CALLBACK_EVENT_PULSE);
    } else {
        _timerID=timeSetEvent(time, 0,(LPTIMECALLBACK)HANDLE(_event),0,
                              TIME_ONESHOT|TIME_CALLBACK_EVENT_SET);
    }

    if (_timerID == NULL)
    {
        return false;
    }
    return true;
}

bool EventWindows::StopTimer()
{
    timeKillEvent(_timerID);
    _timerID = NULL;
    return true;
}
} 
