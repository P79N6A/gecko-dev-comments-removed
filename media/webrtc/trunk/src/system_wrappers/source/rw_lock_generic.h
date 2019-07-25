









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_GENERIC_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_GENERIC_H_

#include "rw_lock_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
class ConditionVariableWrapper;

class RWLockWrapperGeneric : public RWLockWrapper
{
public:
    RWLockWrapperGeneric();
    virtual ~RWLockWrapperGeneric();

    virtual void AcquireLockExclusive();
    virtual void ReleaseLockExclusive();

    virtual void AcquireLockShared();
    virtual void ReleaseLockShared();

protected:
    virtual int Init();

private:
    CriticalSectionWrapper*   _critSectPtr;
    ConditionVariableWrapper* _readCondPtr;
    ConditionVariableWrapper* _writeCondPtr;

    int  _readersActive;
    bool _writerActive;
    int  _readersWaiting;
    int  _writersWaiting;
};
} 
#endif 
