









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_





namespace webrtc {

class RWLockWrapper {
 public:
  static RWLockWrapper* CreateRWLock();
  virtual ~RWLockWrapper() {}

  virtual void AcquireLockExclusive() = 0;
  virtual void ReleaseLockExclusive() = 0;

  virtual void AcquireLockShared() = 0;
  virtual void ReleaseLockShared() = 0;
};



class ReadLockScoped {
 public:
  ReadLockScoped(RWLockWrapper& rw_lock)
    :
    rw_lock_(rw_lock) {
    rw_lock_.AcquireLockShared();
  }

  ~ReadLockScoped() {
    rw_lock_.ReleaseLockShared();
  }

 private:
  RWLockWrapper& rw_lock_;
};

class WriteLockScoped {
 public:
  WriteLockScoped(RWLockWrapper& rw_lock)
    :
    rw_lock_(rw_lock) {
    rw_lock_.AcquireLockExclusive();
  }

  ~WriteLockScoped() {
    rw_lock_.ReleaseLockExclusive();
  }

 private:
  RWLockWrapper& rw_lock_;
};

} 

#endif  
