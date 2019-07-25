









#include "process_thread_impl.h"
#include "module.h"
#include "trace.h"

namespace webrtc {
ProcessThread::~ProcessThread()
{
}

ProcessThread* ProcessThread::CreateProcessThread()
{
    return new ProcessThreadImpl();
}

void ProcessThread::DestroyProcessThread(ProcessThread* module)
{
    delete module;
}

ProcessThreadImpl::ProcessThreadImpl()
    : _timeEvent(*EventWrapper::Create()),
      _critSectModules(CriticalSectionWrapper::CreateCriticalSection()),
      _thread(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceUtility, -1, "%s created", __FUNCTION__);
}

ProcessThreadImpl::~ProcessThreadImpl()
{
    delete _critSectModules;
    delete &_timeEvent;
    WEBRTC_TRACE(kTraceMemory, kTraceUtility, -1, "%s deleted", __FUNCTION__);
}

WebRtc_Word32 ProcessThreadImpl::Start()
{
    CriticalSectionScoped lock(_critSectModules);
    if(_thread)
    {
        return -1;
    }
    _thread = ThreadWrapper::CreateThread(Run, this, kNormalPriority,
                                          "ProcessThread");
    unsigned int id;
    WebRtc_Word32 retVal = _thread->Start(id);
    if(retVal >= 0)
    {
        return 0;
    }
    delete _thread;
    _thread = NULL;
    return -1;
}

WebRtc_Word32 ProcessThreadImpl::Stop()
{
    _critSectModules->Enter();
    if(_thread)
    {
        _thread->SetNotAlive();

        ThreadWrapper* thread = _thread;
        _thread = NULL;

        _timeEvent.Set();
        _critSectModules->Leave();

        if(thread->Stop())
        {
            delete thread;
        } else {
            return -1;
        }
    } else {
        _critSectModules->Leave();
    }
    return 0;
}

WebRtc_Word32 ProcessThreadImpl::RegisterModule(const Module* module)
{
    CriticalSectionScoped lock(_critSectModules);

    
    ListItem* item = _modules.First();
    for(WebRtc_UWord32 i = 0; i < _modules.GetSize() && item; i++)
    {
        if(module == item->GetItem())
        {
            return -1;
        }
        item = _modules.Next(item);
    }

    _modules.PushFront(module);
    WEBRTC_TRACE(kTraceInfo, kTraceUtility, -1,
                 "number of registered modules has increased to %d",
                 _modules.GetSize());
    
    
    
    _timeEvent.Set();
    return 0;
}

WebRtc_Word32 ProcessThreadImpl::DeRegisterModule(const Module* module)
{
    CriticalSectionScoped lock(_critSectModules);

    ListItem* item = _modules.First();
    for(WebRtc_UWord32 i = 0; i < _modules.GetSize() && item; i++)
    {
        if(module == item->GetItem())
        {
            int res = _modules.Erase(item);
            WEBRTC_TRACE(kTraceInfo, kTraceUtility, -1,
                         "number of registered modules has decreased to %d",
                         _modules.GetSize());
            return res;
        }
        item = _modules.Next(item);
    }
    return -1;
}

bool ProcessThreadImpl::Run(void* obj)
{
    return static_cast<ProcessThreadImpl*>(obj)->Process();
}

bool ProcessThreadImpl::Process()
{
    
    
    WebRtc_Word32 minTimeToNext = 100;
    {
        CriticalSectionScoped lock(_critSectModules);
        ListItem* item = _modules.First();
        for(WebRtc_UWord32 i = 0; i < _modules.GetSize() && item; i++)
        {
            WebRtc_Word32 timeToNext =
                static_cast<Module*>(item->GetItem())->TimeUntilNextProcess();
            if(minTimeToNext > timeToNext)
            {
                minTimeToNext = timeToNext;
            }
            item = _modules.Next(item);
        }
    }

    if(minTimeToNext > 0)
    {
        if(kEventError == _timeEvent.Wait(minTimeToNext))
        {
            return true;
        }
        if(!_thread)
        {
            return false;
        }
    }
    {
        CriticalSectionScoped lock(_critSectModules);
        ListItem* item = _modules.First();
        for(WebRtc_UWord32 i = 0; i < _modules.GetSize() && item; i++)
        {
            WebRtc_Word32 timeToNext =
                static_cast<Module*>(item->GetItem())->TimeUntilNextProcess();
            if(timeToNext < 1)
            {
                static_cast<Module*>(item->GetItem())->Process();
            }
            item = _modules.Next(item);
        }
    }
    return true;
}
} 
