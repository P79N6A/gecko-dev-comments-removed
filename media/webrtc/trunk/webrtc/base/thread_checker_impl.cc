











#include "webrtc/base/thread_checker_impl.h"

#include "webrtc/base/thread.h"

namespace rtc {

ThreadCheckerImpl::ThreadCheckerImpl()
    : valid_thread_() {
  EnsureThreadIdAssigned();
}

ThreadCheckerImpl::~ThreadCheckerImpl() {
}

bool ThreadCheckerImpl::CalledOnValidThread() const {
  CritScope scoped_lock(&lock_);
  EnsureThreadIdAssigned();
  return valid_thread_->IsCurrent();
}

void ThreadCheckerImpl::DetachFromThread() {
  CritScope scoped_lock(&lock_);
  valid_thread_ = NULL;
}

void ThreadCheckerImpl::EnsureThreadIdAssigned() const {
  if (!valid_thread_) {
    valid_thread_ = Thread::Current();
  }
}

}  
