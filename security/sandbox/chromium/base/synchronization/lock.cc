







#if !defined(NDEBUG)

#include "base/synchronization/lock.h"
#include "base/logging.h"

namespace base {

const PlatformThreadId kNoThreadId = static_cast<PlatformThreadId>(0);

Lock::Lock() : lock_() {
  owned_by_thread_ = false;
  owning_thread_id_ = kNoThreadId;
}

Lock::~Lock() {
  DCHECK(!owned_by_thread_);
  DCHECK_EQ(kNoThreadId, owning_thread_id_);
}

void Lock::AssertAcquired() const {
  DCHECK(owned_by_thread_);
  DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
}

void Lock::CheckHeldAndUnmark() {
  DCHECK(owned_by_thread_);
  DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
  owned_by_thread_ = false;
  owning_thread_id_ = kNoThreadId;
}

void Lock::CheckUnheldAndMark() {
  DCHECK(!owned_by_thread_);
  owned_by_thread_ = true;
  owning_thread_id_ = PlatformThread::CurrentId();
}

}  

#endif  
