



#ifndef BASE_NON_THREAD_SAFE_H__
#define BASE_NON_THREAD_SAFE_H__

#include "base/platform_thread.h"



















#ifndef NDEBUG
class NonThreadSafe {
 public:
  NonThreadSafe();
  ~NonThreadSafe();

  bool CalledOnValidThread() const;

 private:
  PlatformThreadId valid_thread_id_;
};
#else

class NonThreadSafe {
 public:
  NonThreadSafe() {}
  ~NonThreadSafe() {}

  bool CalledOnValidThread() const {
    return true;
  }
};
#endif  

#endif  
