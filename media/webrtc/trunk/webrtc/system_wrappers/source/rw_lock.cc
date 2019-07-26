









#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"

#include <assert.h>

#if defined(_WIN32)
#include "webrtc/system_wrappers/source/rw_lock_generic.h"
#include "webrtc/system_wrappers/source/rw_lock_win.h"
#elif defined(ANDROID)
#include "webrtc/system_wrappers/source/rw_lock_generic.h"
#else
#include "webrtc/system_wrappers/source/rw_lock_posix.h"
#endif

namespace webrtc {

RWLockWrapper* RWLockWrapper::CreateRWLock() {
#ifdef _WIN32
  
  RWLockWrapper* lock = RWLockWin::Create();
  if (lock) {
    return lock;
  }
  return new RWLockGeneric();
#elif defined(ANDROID)
  
  return new RWLockGeneric();
#else
  return RWLockPosix::Create();
#endif
}

}  
