



#include "base/lock_impl.h"
#include "base/logging.h"





LockImpl::LockImpl() {
#ifndef NDEBUG
  recursion_count_shadow_ = 0;
  recursion_used_ = false;
  owning_thread_id_ = 0;
#endif  
  
  
  ::InitializeCriticalSectionAndSpinCount(&os_lock_, 2000);
}

LockImpl::~LockImpl() {
  ::DeleteCriticalSection(&os_lock_);
}

bool LockImpl::Try() {
  if (::TryEnterCriticalSection(&os_lock_) != FALSE) {
#ifndef NDEBUG
    
    owning_thread_id_ = PlatformThread::CurrentId();
    DCHECK_NE(owning_thread_id_, 0);
    recursion_count_shadow_++;
    if (2 == recursion_count_shadow_ && !recursion_used_) {
      recursion_used_ = true;
      DCHECK(false);  
    }
#endif
    return true;
  }
  return false;
}

void LockImpl::Lock() {
  ::EnterCriticalSection(&os_lock_);
#ifndef NDEBUG
  
  owning_thread_id_ = PlatformThread::CurrentId();
  DCHECK_NE(owning_thread_id_, 0);
  recursion_count_shadow_++;
  if (2 == recursion_count_shadow_ && !recursion_used_) {
    recursion_used_ = true;
    DCHECK(false);  
  }
#endif  
}

void LockImpl::Unlock() {
#ifndef NDEBUG
  --recursion_count_shadow_;  
  DCHECK(0 <= recursion_count_shadow_);
  owning_thread_id_ = 0;
#endif  
  ::LeaveCriticalSection(&os_lock_);
}


#ifndef NDEBUG
void LockImpl::AssertAcquired() const {
  DCHECK(recursion_count_shadow_ > 0);
  DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
}
#endif
