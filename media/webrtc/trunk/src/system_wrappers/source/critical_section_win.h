









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_WINDOWS_H_

#include "typedefs.h"
#include "critical_section_wrapper.h"
#include <windows.h>

namespace webrtc {
class CriticalSectionWindows : public CriticalSectionWrapper
{
public:
    CriticalSectionWindows();

    virtual ~CriticalSectionWindows();

    virtual void Enter();
    virtual void Leave();

private:
    CRITICAL_SECTION crit;

    friend class ConditionVariableWindows;
};
} 

#endif 
