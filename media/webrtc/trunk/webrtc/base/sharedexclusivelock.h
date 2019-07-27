









#ifndef WEBRTC_BASE_SHAREDEXCLUSIVELOCK_H_
#define WEBRTC_BASE_SHAREDEXCLUSIVELOCK_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/event.h"

namespace rtc {



class LOCKABLE SharedExclusiveLock {
 public:
  SharedExclusiveLock();

  
  
  void LockExclusive() EXCLUSIVE_LOCK_FUNCTION();
  void UnlockExclusive() UNLOCK_FUNCTION();
  void LockShared();
  void UnlockShared();

 private:
  rtc::CriticalSection cs_exclusive_;
  rtc::CriticalSection cs_shared_;
  rtc::Event shared_count_is_zero_;
  int shared_count_;

  DISALLOW_COPY_AND_ASSIGN(SharedExclusiveLock);
};

class SCOPED_LOCKABLE SharedScope {
 public:
  explicit SharedScope(SharedExclusiveLock* lock) SHARED_LOCK_FUNCTION(lock)
      : lock_(lock) {
    lock_->LockShared();
  }

  ~SharedScope() UNLOCK_FUNCTION() { lock_->UnlockShared(); }

 private:
  SharedExclusiveLock* lock_;

  DISALLOW_COPY_AND_ASSIGN(SharedScope);
};

class SCOPED_LOCKABLE ExclusiveScope {
 public:
  explicit ExclusiveScope(SharedExclusiveLock* lock)
      EXCLUSIVE_LOCK_FUNCTION(lock)
      : lock_(lock) {
    lock_->LockExclusive();
  }

  ~ExclusiveScope() UNLOCK_FUNCTION() { lock_->UnlockExclusive(); }

 private:
  SharedExclusiveLock* lock_;

  DISALLOW_COPY_AND_ASSIGN(ExclusiveScope);
};

}  

#endif  
