



#ifndef RWLOCK_AUTO_ENTER_H
#define RWLOCK_AUTO_ENTER_H

#include "prrwlock.h"
#include "mozilla/Assertions.h"

class RwLockAutoEnterRead
{
public:
  explicit RwLockAutoEnterRead(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    MOZ_ASSERT(mRwLock);
    PR_RWLock_Rlock(mRwLock);
  }

  ~RwLockAutoEnterRead()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock* mRwLock;
};

class RwLockAutoEnterWrite
{
public:
  explicit RwLockAutoEnterWrite(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    MOZ_ASSERT(mRwLock);
    PR_RWLock_Wlock(mRwLock);
  }

  ~RwLockAutoEnterWrite()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock* mRwLock;
};

#endif 
