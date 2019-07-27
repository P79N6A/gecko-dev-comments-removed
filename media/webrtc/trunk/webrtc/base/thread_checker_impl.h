











#ifndef WEBRTC_BASE_THREAD_CHECKER_IMPL_H_
#define WEBRTC_BASE_THREAD_CHECKER_IMPL_H_

#include "webrtc/base/criticalsection.h"

namespace rtc {

class Thread;







class ThreadCheckerImpl {
 public:
  ThreadCheckerImpl();
  ~ThreadCheckerImpl();

  bool CalledOnValidThread() const;

  
  
  
  void DetachFromThread();

 private:
  void EnsureThreadIdAssigned() const;

  mutable CriticalSection lock_;
  
  
  mutable const Thread* valid_thread_;
};

}  

#endif  
