









#include "rw_lock_posix.h"

namespace webrtc {
RWLockPosix::RWLockPosix() : _lock()
{
}

RWLockPosix::~RWLockPosix()
{
    pthread_rwlock_destroy(&_lock);
}

int RWLockPosix::Init()
{
    return pthread_rwlock_init(&_lock, 0);
}

void RWLockPosix::AcquireLockExclusive()
{
    pthread_rwlock_wrlock(&_lock);
}

void RWLockPosix::ReleaseLockExclusive()
{
    pthread_rwlock_unlock(&_lock);
}

void RWLockPosix::AcquireLockShared()
{
    pthread_rwlock_rdlock(&_lock);
}

void RWLockPosix::ReleaseLockShared()
{
    pthread_rwlock_unlock(&_lock);
}
} 
