




































#pragma once

#include <CarbonEvents.h>

class TimedMessage {
public:
    
    void* operator new(size_t n);
    void operator delete(void* ptr);
    
    TimedMessage();
    virtual ~TimedMessage();
    
    OSStatus send();
    virtual void execute() = 0;

private:
    static pascal void TimedMessageHandler(EventLoopTimerRef inTimer, void *inUserData);
    EventLoopTimerUPP mTimerUPP;
};
