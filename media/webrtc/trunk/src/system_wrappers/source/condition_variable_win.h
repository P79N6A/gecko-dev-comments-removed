









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CONDITION_VARIABLE_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CONDITION_VARIABLE_WINDOWS_H_

#include "condition_variable_wrapper.h"

#include <windows.h>

namespace webrtc {
#if !defined CONDITION_VARIABLE_INIT
    typedef struct _RTL_CONDITION_VARIABLE
    {
        void* Ptr;
    } RTL_CONDITION_VARIABLE, *PRTL_CONDITION_VARIABLE;

    typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;
#endif

typedef void (WINAPI *PInitializeConditionVariable)(PCONDITION_VARIABLE);
typedef BOOL (WINAPI *PSleepConditionVariableCS)(PCONDITION_VARIABLE,
                                                 PCRITICAL_SECTION, DWORD);
typedef void (WINAPI *PWakeConditionVariable)(PCONDITION_VARIABLE);
typedef void (WINAPI *PWakeAllConditionVariable)(PCONDITION_VARIABLE);


class ConditionVariableWindows : public ConditionVariableWrapper
{
public:
    ConditionVariableWindows();
    ~ConditionVariableWindows();

    void SleepCS(CriticalSectionWrapper& critSect);
    bool SleepCS(CriticalSectionWrapper& critSect, unsigned long maxTimeInMS);
    void Wake();
    void WakeAll();

private:
    enum EventWakeUpType
    {
        WAKEALL_0   = 0,
        WAKEALL_1   = 1,
        WAKE        = 2,
        EVENT_COUNT = 3
    };

private:
    
    static bool              _winSupportConditionVariablesPrimitive;
    CONDITION_VARIABLE       _conditionVariable;

    unsigned int     _numWaiters[2];
    EventWakeUpType  _eventID;
    CRITICAL_SECTION _numWaitersCritSect;
    HANDLE           _events[EVENT_COUNT];
};
} 

#endif
