









#include "rw_lock_generic.h"

#include "condition_variable_wrapper.h"
#include "critical_section_wrapper.h"

namespace webrtc {
RWLockWrapperGeneric::RWLockWrapperGeneric()
    : _readersActive(0),
      _writerActive(false),
      _readersWaiting(0),
      _writersWaiting(0)
{
    _critSectPtr  = CriticalSectionWrapper::CreateCriticalSection();
    _readCondPtr  = ConditionVariableWrapper::CreateConditionVariable();
    _writeCondPtr = ConditionVariableWrapper::CreateConditionVariable();
}

RWLockWrapperGeneric::~RWLockWrapperGeneric()
{
    delete _writeCondPtr;
    delete _readCondPtr;
    delete _critSectPtr;
}

int RWLockWrapperGeneric::Init()
{
    return 0;
}

void RWLockWrapperGeneric::AcquireLockExclusive()
{
    _critSectPtr->Enter();

    if (_writerActive || _readersActive > 0)
    {
        ++_writersWaiting;

        while (_writerActive || _readersActive > 0)
        {
            _writeCondPtr->SleepCS(*_critSectPtr);
        }

        --_writersWaiting;
    }
    _writerActive = true;
    _critSectPtr->Leave();
}

void RWLockWrapperGeneric::ReleaseLockExclusive()
{
    _critSectPtr->Enter();

    _writerActive = false;

    if (_writersWaiting > 0)
    {
        _writeCondPtr->Wake();

    }else if (_readersWaiting > 0)
    {
        _readCondPtr->WakeAll();
    }
    _critSectPtr->Leave();
}

void RWLockWrapperGeneric::AcquireLockShared()
{
    _critSectPtr->Enter();

    if (_writerActive || _writersWaiting > 0)
    {
        ++_readersWaiting;

        while (_writerActive || _writersWaiting > 0)
        {
            _readCondPtr->SleepCS(*_critSectPtr);
        }
        --_readersWaiting;
    }
    ++_readersActive;
    _critSectPtr->Leave();
}

void RWLockWrapperGeneric::ReleaseLockShared()
{
    _critSectPtr->Enter();

    --_readersActive;

    if (_readersActive == 0 && _writersWaiting > 0)
    {
        _writeCondPtr->Wake();
    }
    _critSectPtr->Leave();
}
} 
