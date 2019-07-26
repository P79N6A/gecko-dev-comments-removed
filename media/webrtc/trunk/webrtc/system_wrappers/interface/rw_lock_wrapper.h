









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_

#include "webrtc/system_wrappers/interface/thread_annotations.h"





namespace webrtc {

class LOCKABLE RWLockWrapper {
 public:
  static RWLockWrapper* CreateRWLock();
  virtual ~RWLockWrapper() {}

  virtual void AcquireLockExclusive() EXCLUSIVE_LOCK_FUNCTION() = 0;
  virtual void ReleaseLockExclusive() UNLOCK_FUNCTION() = 0;

  virtual void AcquireLockShared() SHARED_LOCK_FUNCTION() = 0;
  virtual void ReleaseLockShared() UNLOCK_FUNCTION() = 0;
};



class SCOPED_LOCKABLE ReadLockScoped {
 public:
  ReadLockScoped(RWLockWrapper& rw_lock) SHARED_LOCK_FUNCTION(rw_lock)
      : rw_lock_(rw_lock) {
    rw_lock_.AcquireLockShared();
  }

  ~ReadLockScoped() UNLOCK_FUNCTION() {
    rw_lock_.ReleaseLockShared();
  }

 private:
  RWLockWrapper& rw_lock_;
};

class SCOPED_LOCKABLE WriteLockScoped {
 public:
  WriteLockScoped(RWLockWrapper& rw_lock) EXCLUSIVE_LOCK_FUNCTION(rw_lock)
      : rw_lock_(rw_lock) {
    rw_lock_.AcquireLockExclusive();
  }

  ~WriteLockScoped() UNLOCK_FUNCTION() {
    rw_lock_.ReleaseLockExclusive();
  }

 private:
  RWLockWrapper& rw_lock_;
};

}  

#endif  
