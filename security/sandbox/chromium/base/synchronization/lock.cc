







#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)

#include "base/synchronization/lock.h"
#include "base/logging.h"

namespace base {

Lock::Lock() : lock_() {
}

Lock::~Lock() {
  DCHECK(owning_thread_ref_.is_null());
}

void Lock::AssertAcquired() const {
  DCHECK(owning_thread_ref_ == PlatformThread::CurrentRef());
}

void Lock::CheckHeldAndUnmark() {
  DCHECK(owning_thread_ref_ == PlatformThread::CurrentRef());
  owning_thread_ref_ = PlatformThreadRef();
}

void Lock::CheckUnheldAndMark() {
  DCHECK(owning_thread_ref_.is_null());
  owning_thread_ref_ = PlatformThread::CurrentRef();
}

}  

#endif  
