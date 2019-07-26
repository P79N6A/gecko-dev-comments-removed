









#include "thread_win.h"

#include <assert.h>
#include <process.h>
#include <stdio.h>
#include <windows.h>

#include "set_thread_name_win.h"
#include "trace.h"

namespace webrtc {
ThreadWindows::ThreadWindows(ThreadRunFunction func, ThreadObj obj,
                             ThreadPriority prio, const char* threadName)
    : ThreadWrapper(),
      _runFunction(func),
      _obj(obj),
      _alive(false),
      _dead(true),
      _doNotCloseHandle(false),
      _prio(prio),
      _event(NULL),
      _thread(NULL),
      _id(0),
      _name(),
      _setThreadName(false)
{
    _event = EventWrapper::Create();
    _critsectStop = CriticalSectionWrapper::CreateCriticalSection();
    if (threadName != NULL)
    {
        
        _setThreadName = true;
        strncpy(_name, threadName, kThreadMaxNameLength);
    }
}

ThreadWindows::~ThreadWindows()
{
#ifdef _DEBUG
    assert(!_alive);
#endif
    if (_thread)
    {
        CloseHandle(_thread);
    }
    if(_event)
    {
        delete _event;
    }
    if(_critsectStop)
    {
        delete _critsectStop;
    }
}

uint32_t ThreadWrapper::GetThreadId() {
  return GetCurrentThreadId();
}

unsigned int WINAPI ThreadWindows::StartThread(LPVOID lpParameter)
{
    static_cast<ThreadWindows*>(lpParameter)->Run();
    return 0;
}

bool ThreadWindows::Start(unsigned int& threadID)
{
    _doNotCloseHandle = false;

    
    _thread=(HANDLE)_beginthreadex(NULL, 1024*1024, StartThread, (void*)this, 0,
                                   &threadID);
    if(_thread == NULL)
    {
        return false;
    }
    _id = threadID;
    _event->Wait(INFINITE);

    switch(_prio)
    {
    case kLowPriority:
        SetThreadPriority(_thread, THREAD_PRIORITY_BELOW_NORMAL);
        break;
    case kNormalPriority:
        SetThreadPriority(_thread, THREAD_PRIORITY_NORMAL);
        break;
    case kHighPriority:
        SetThreadPriority(_thread, THREAD_PRIORITY_ABOVE_NORMAL);
        break;
    case kHighestPriority:
        SetThreadPriority(_thread, THREAD_PRIORITY_HIGHEST);
        break;
    case kRealtimePriority:
        SetThreadPriority(_thread, THREAD_PRIORITY_TIME_CRITICAL);
        break;
    };
    return true;
}

bool ThreadWindows::SetAffinity(const int* processorNumbers,
                                const unsigned int amountOfProcessors)
{
    DWORD_PTR processorBitMask = 0;
    for(unsigned int processorIndex = 0;
        processorIndex < amountOfProcessors;
        processorIndex++)
    {
        
        
        
        
        processorBitMask = 1 << processorNumbers[processorIndex];
    }
    return SetThreadAffinityMask(_thread,processorBitMask) != 0;
}

void ThreadWindows::SetNotAlive()
{
    _alive = false;
}

bool ThreadWindows::Shutdown()
{
    DWORD exitCode = 0;
    BOOL ret = TRUE;
    if (_thread)
    {
        ret = TerminateThread(_thread, exitCode);
        _alive = false;
        _dead = true;
        _thread = NULL;
    }
    return ret == TRUE;
}

bool ThreadWindows::Stop()
{
    _critsectStop->Enter();
    
    _doNotCloseHandle = true;
    _alive = false;
    bool signaled = false;
    if (_thread && !_dead)
    {
        _critsectStop->Leave();
        
        if( WAIT_OBJECT_0 == WaitForSingleObject(_thread, 2000))
        {
            signaled = true;
        }
        _critsectStop->Enter();
    }
    if (_thread)
    {
        CloseHandle(_thread);
        _thread = NULL;
    }
    _critsectStop->Leave();

    if (_dead || signaled)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ThreadWindows::Run()
{
    _alive = true;
    _dead = false;
    _event->Set();

    
    if (_setThreadName)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, _id,
                     "Thread with name:%s started ", _name);
        SetThreadName(-1, _name); 
    }else
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, _id,
                     "Thread without name started");
    }

    do
    {
        if (_runFunction)
        {
            if (!_runFunction(_obj))
            {
                _alive = false;
            }
        } else {
            _alive = false;
        }
    } while(_alive);

    if (_setThreadName)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, _id,
                     "Thread with name:%s stopped", _name);
    } else {
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,_id,
                     "Thread without name stopped");
    }

    _critsectStop->Enter();

    if (_thread && !_doNotCloseHandle)
    {
        HANDLE thread = _thread;
        _thread = NULL;
        CloseHandle(thread);
    }
    _dead = true;

    _critsectStop->Leave();
};
} 
