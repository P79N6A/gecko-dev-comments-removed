









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_WINDOWS_H_

#include <windows.h>

#include "event_wrapper.h"

#include "typedefs.h"

namespace webrtc {
class EventWindows : public EventWrapper
{
public:
    EventWindows();
    virtual ~EventWindows();

    virtual EventTypeWrapper Wait(unsigned long maxTime);
    virtual bool Set();
    virtual bool Reset();

    virtual bool StartTimer(bool periodic, unsigned long time);
    virtual bool StopTimer();

private:
    HANDLE  _event;
    WebRtc_UWord32 _timerID;
};
} 

#endif 
