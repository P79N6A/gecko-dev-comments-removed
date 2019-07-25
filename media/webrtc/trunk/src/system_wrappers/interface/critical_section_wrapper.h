









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_




#include "common_types.h"

namespace webrtc {
class CriticalSectionWrapper
{
public:
    
    static CriticalSectionWrapper* CreateCriticalSection();

    virtual ~CriticalSectionWrapper() {}

    
    
    virtual void Enter() = 0;

    
    virtual void Leave() = 0;
};



class CriticalSectionScoped
{
public:
    
    
    
    explicit CriticalSectionScoped(CriticalSectionWrapper& critsec)
        : _ptrCritSec(&critsec)
    {
        _ptrCritSec->Enter();
    }

    explicit CriticalSectionScoped(CriticalSectionWrapper* critsec)
        : _ptrCritSec(critsec)
    {
      _ptrCritSec->Enter();
    }

    ~CriticalSectionScoped()
    {
        if (_ptrCritSec)
        {
            Leave();
        }
    }

private:
    void Leave()
    {
        _ptrCritSec->Leave();
        _ptrCritSec = 0;
    }

    CriticalSectionWrapper* _ptrCritSec;
};
} 
#endif 
