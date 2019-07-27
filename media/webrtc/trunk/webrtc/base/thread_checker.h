











#ifndef WEBRTC_BASE_THREAD_CHECKER_H_
#define WEBRTC_BASE_THREAD_CHECKER_H_










#if (!defined(NDEBUG) || defined(DCHECK_ALWAYS_ON))
#define ENABLE_THREAD_CHECKER 1
#else
#define ENABLE_THREAD_CHECKER 0
#endif

#include "webrtc/base/thread_checker_impl.h"

namespace rtc {





class ThreadCheckerDoNothing {
 public:
  bool CalledOnValidThread() const {
    return true;
  }

  void DetachFromThread() {}
};
































#if ENABLE_THREAD_CHECKER
class ThreadChecker : public ThreadCheckerImpl {
};
#else
class ThreadChecker : public ThreadCheckerDoNothing {
};
#endif  

#undef ENABLE_THREAD_CHECKER

}  

#endif  
