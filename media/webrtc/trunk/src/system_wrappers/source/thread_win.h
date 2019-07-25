









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WINDOWS_H_

#include "thread_wrapper.h"
#include "event_wrapper.h"
#include "critical_section_wrapper.h"

#include <windows.h>

namespace webrtc {

class ThreadWindows : public ThreadWrapper
{
public:
    ThreadWindows(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio,
                  const char* threadName);
    virtual ~ThreadWindows();

    virtual bool Start(unsigned int& id);
    bool SetAffinity(const int* processorNumbers,
                     const unsigned int amountOfProcessors);
    virtual bool Stop();
    virtual void SetNotAlive();

    static unsigned int WINAPI StartThread(LPVOID lpParameter);

    virtual bool Shutdown();

protected:
    virtual void Run();

private:
    ThreadRunFunction    _runFunction;
    ThreadObj            _obj;

    bool                    _alive;
    bool                    _dead;

    
    
    
    
    bool                    _doNotCloseHandle;
    ThreadPriority          _prio;
    EventWrapper*           _event;
    CriticalSectionWrapper* _critsectStop;

    HANDLE                  _thread;
    unsigned int            _id;
    char                    _name[kThreadMaxNameLength];
    bool                    _setThreadName;

};
} 

#endif 
