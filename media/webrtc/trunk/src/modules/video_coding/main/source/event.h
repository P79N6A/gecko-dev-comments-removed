









#ifndef WEBRTC_MODULES_VIDEO_CODING_EVENT_H_
#define WEBRTC_MODULES_VIDEO_CODING_EVENT_H_

#include "event_wrapper.h"

namespace webrtc
{



class VCMEvent : public EventWrapper
{
public:
    VCMEvent() : _event(*EventWrapper::Create()) {};

    virtual ~VCMEvent() { delete &_event; };

    


    bool Set() { return _event.Set(); };

    bool Reset() { return _event.Reset(); };

    


    EventTypeWrapper Wait(unsigned long maxTime)
    {
#ifdef EVENT_DEBUG
        return kEventTimeout;
#else
        return _event.Wait(maxTime);
#endif
    };

    


    bool StartTimer(bool periodic, unsigned long time)
                   { return _event.StartTimer(periodic, time); };
    


    bool StopTimer() { return _event.StopTimer(); };

private:
    EventWrapper&      _event;
};

} 

#endif 
