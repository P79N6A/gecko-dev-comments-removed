









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_POSIX_H_

#include "rw_lock_wrapper.h"

#include <pthread.h>

namespace webrtc {
class RWLockPosix : public RWLockWrapper
{
public:
    RWLockPosix();
    virtual ~RWLockPosix();

    virtual void AcquireLockExclusive();
    virtual void ReleaseLockExclusive();

    virtual void AcquireLockShared();
    virtual void ReleaseLockShared();

protected:
    virtual int Init();

private:
    pthread_rwlock_t _lock;
};
} 

#endif 
