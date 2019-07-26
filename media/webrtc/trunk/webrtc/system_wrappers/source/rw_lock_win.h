









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_WIN_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_WIN_H_

#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"

#include <Windows.h>

namespace webrtc {

class RWLockWin : public RWLockWrapper {
 public:
  static RWLockWin* Create();
  ~RWLockWin() {}

  virtual void AcquireLockExclusive();
  virtual void ReleaseLockExclusive();

  virtual void AcquireLockShared();
  virtual void ReleaseLockShared();

 private:
  RWLockWin();
  static bool LoadModule();

  SRWLOCK lock_;
};

}  

#endif  
