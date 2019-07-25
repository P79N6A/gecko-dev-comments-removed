



















#include "condition_variable_win.h"

#include "critical_section_win.h"
#include "trace.h"

namespace webrtc {
bool ConditionVariableWindows::_winSupportConditionVariablesPrimitive = false;
static HMODULE library = NULL;

PInitializeConditionVariable  _PInitializeConditionVariable;
PSleepConditionVariableCS     _PSleepConditionVariableCS;
PWakeConditionVariable        _PWakeConditionVariable;
PWakeAllConditionVariable     _PWakeAllConditionVariable;

typedef void (WINAPI *PInitializeConditionVariable)(PCONDITION_VARIABLE);
typedef BOOL (WINAPI *PSleepConditionVariableCS)(PCONDITION_VARIABLE,
                                                 PCRITICAL_SECTION, DWORD);
typedef void (WINAPI *PWakeConditionVariable)(PCONDITION_VARIABLE);
typedef void (WINAPI *PWakeAllConditionVariable)(PCONDITION_VARIABLE);

ConditionVariableWindows::ConditionVariableWindows()
    : _eventID(WAKEALL_0)
{
    if (!library)
    {
        
        library = LoadLibrary(TEXT("Kernel32.dll"));
        if (library)
        {
            WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                         "Loaded Kernel.dll");

            _PInitializeConditionVariable =
                (PInitializeConditionVariable) GetProcAddress(
                    library,
                    "InitializeConditionVariable");
            _PSleepConditionVariableCS =
                (PSleepConditionVariableCS)GetProcAddress(
                    library,
                    "SleepConditionVariableCS");
            _PWakeConditionVariable =
                (PWakeConditionVariable)GetProcAddress(
                    library,
                     "WakeConditionVariable");
            _PWakeAllConditionVariable =
                (PWakeAllConditionVariable)GetProcAddress(
                    library,
                    "WakeAllConditionVariable");

            if(_PInitializeConditionVariable &&
               _PSleepConditionVariableCS &&
               _PWakeConditionVariable &&
               _PWakeAllConditionVariable)
            {
                WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                             "Loaded native condition variables");
                _winSupportConditionVariablesPrimitive = true;
            }
        }
    }

    if (_winSupportConditionVariablesPrimitive)
    {
        _PInitializeConditionVariable(&_conditionVariable);

        _events[WAKEALL_0] = NULL;
        _events[WAKEALL_1] = NULL;
        _events[WAKE] = NULL;

    } else {
        memset(&_numWaiters[0],0,sizeof(_numWaiters));

        InitializeCriticalSection(&_numWaitersCritSect);

        _events[WAKEALL_0] = CreateEvent(NULL,  
                                         TRUE,  
                                         FALSE, 
                                         NULL); 

        _events[WAKEALL_1] = CreateEvent(NULL,  
                                         TRUE,  
                                         FALSE, 
                                         NULL); 

        _events[WAKE] = CreateEvent(NULL,  
                                    FALSE, 
                                    FALSE, 
                                    NULL); 
    }
}

ConditionVariableWindows::~ConditionVariableWindows()
{
    if(!_winSupportConditionVariablesPrimitive)
    {
        CloseHandle(_events[WAKE]);
        CloseHandle(_events[WAKEALL_1]);
        CloseHandle(_events[WAKEALL_0]);

        DeleteCriticalSection(&_numWaitersCritSect);
    }
}

void ConditionVariableWindows::SleepCS(CriticalSectionWrapper& critSect)
{
    SleepCS(critSect, INFINITE);
}

bool ConditionVariableWindows::SleepCS(CriticalSectionWrapper& critSect,
                                       unsigned long maxTimeInMS)
{
    CriticalSectionWindows* cs = reinterpret_cast<CriticalSectionWindows*>(
                                     &critSect);

    if(_winSupportConditionVariablesPrimitive)
    {
        BOOL retVal = _PSleepConditionVariableCS(&_conditionVariable,
                                                 &(cs->crit),maxTimeInMS);
        return (retVal == 0) ? false : true;

    }else
    {
        EnterCriticalSection(&_numWaitersCritSect);
        
        
        const EventWakeUpType eventID = (WAKEALL_0 == _eventID) ?
                                            WAKEALL_1 : WAKEALL_0;
        ++(_numWaiters[eventID]);
        LeaveCriticalSection(&_numWaitersCritSect);

        LeaveCriticalSection(&cs->crit);
        HANDLE events[2];
        events[0] = _events[WAKE];
        events[1] = _events[eventID];
        const DWORD result = WaitForMultipleObjects(2, 
                                                    events,
                                                    FALSE, 
                                                    maxTimeInMS);

        const bool retVal = (result != WAIT_TIMEOUT);

        EnterCriticalSection(&_numWaitersCritSect);
        --(_numWaiters[eventID]);
        
        
        const bool lastWaiter = (result == WAIT_OBJECT_0 + 1) &&
                                (_numWaiters[eventID] == 0);
        LeaveCriticalSection(&_numWaitersCritSect);

        if (lastWaiter)
        {
            
            
            ResetEvent(_events[eventID]);
        }

        EnterCriticalSection(&cs->crit);
        return retVal;
    }
}

void
ConditionVariableWindows::Wake()
{
    if(_winSupportConditionVariablesPrimitive)
    {
        _PWakeConditionVariable(&_conditionVariable);
    }else
    {
        EnterCriticalSection(&_numWaitersCritSect);
        const bool haveWaiters = (_numWaiters[WAKEALL_0] > 0) ||
                                 (_numWaiters[WAKEALL_1] > 0);
        LeaveCriticalSection(&_numWaitersCritSect);

        if (haveWaiters)
        {
            SetEvent(_events[WAKE]);
        }
    }
}

void
ConditionVariableWindows::WakeAll()
{
    if(_winSupportConditionVariablesPrimitive)
    {
        _PWakeAllConditionVariable(&_conditionVariable);
    }else
    {
        EnterCriticalSection(&_numWaitersCritSect);
        
        _eventID = (WAKEALL_0 == _eventID) ? WAKEALL_1 : WAKEALL_0;
        
        const EventWakeUpType eventID = _eventID;
        const bool haveWaiters = _numWaiters[eventID] > 0;
        LeaveCriticalSection(&_numWaitersCritSect);

        if (haveWaiters)
        {
            SetEvent(_events[eventID]);
        }
    }
}
} 
