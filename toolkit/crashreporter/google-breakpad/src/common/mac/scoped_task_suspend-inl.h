































#ifndef GOOGLE_BREAKPAD_COMMON_MAC_SCOPED_TASK_SUSPEND_H_
#define GOOGLE_BREAKPAD_COMMON_MAC_SCOPED_TASK_SUSPEND_H_

#include <mach/mach.h>

namespace google_breakpad {

class ScopedTaskSuspend {
 public:
  explicit ScopedTaskSuspend(mach_port_t target) : target_(target) {
    task_suspend(target_);
  }

  ~ScopedTaskSuspend() {
    task_resume(target_);
  }

 private:
  mach_port_t target_;
};

}  

#endif  
