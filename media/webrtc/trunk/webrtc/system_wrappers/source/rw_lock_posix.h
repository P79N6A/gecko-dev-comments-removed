









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_POSIX_H_

#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"

#include <pthread.h>

namespace webrtc {

class RWLockPosix : public RWLockWrapper {
 public:
  static RWLockPosix* Create();
  virtual ~RWLockPosix();

  virtual void AcquireLockExclusive();
  virtual void ReleaseLockExclusive();

  virtual void AcquireLockShared();
  virtual void ReleaseLockShared();

 private:
  RWLockPosix();
  bool Init();

  pthread_rwlock_t lock_;
};

}  

#endif  
