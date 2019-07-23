



#include "base/non_thread_safe.h"


#ifndef NDEBUG

#include "base/logging.h"

NonThreadSafe::NonThreadSafe()
    : valid_thread_id_(PlatformThread::CurrentId()) {
}

bool NonThreadSafe::CalledOnValidThread() const {
  return valid_thread_id_ == PlatformThread::CurrentId();
}

NonThreadSafe::~NonThreadSafe() {
  DCHECK(CalledOnValidThread());
}

#endif  
