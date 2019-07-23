



#ifndef BASE_LOCK_H_
#define BASE_LOCK_H_

#include "base/lock_impl.h"



class Lock {
 public:
  Lock() : lock_() {}
  ~Lock() {}
  void Acquire() { lock_.Lock(); }
  void Release() { lock_.Unlock(); }
  
  
  bool Try() { return lock_.Try(); }

  
  
  
  
  void AssertAcquired() const { return lock_.AssertAcquired(); }

  
  
  
  LockImpl* lock_impl() { return &lock_; }

 private:
  LockImpl lock_;  

  DISALLOW_COPY_AND_ASSIGN(Lock);
};


class AutoLock {
 public:
  explicit AutoLock(Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  ~AutoLock() {
    lock_.AssertAcquired();
    lock_.Release();
  }

 private:
  Lock& lock_;
  DISALLOW_COPY_AND_ASSIGN(AutoLock);
};



class AutoUnlock {
 public:
  explicit AutoUnlock(Lock& lock) : lock_(lock) {
    
    lock_.AssertAcquired();
    lock_.Release();
  }

  ~AutoUnlock() {
    lock_.Acquire();
  }

 private:
  Lock& lock_;
  DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

#endif  
