



#include "base/threading/platform_thread.h"

#include <errno.h>
#include <sched.h>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/safe_strerror_posix.h"
#include "base/threading/thread_id_name_manager.h"
#include "base/threading/thread_restrictions.h"
#include "base/tracked_objects.h"

#if !defined(OS_NACL)
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace base {

namespace {

int ThreadNiceValue(ThreadPriority priority) {
  switch (priority) {
    case kThreadPriority_RealtimeAudio:
      return -10;
    case kThreadPriority_Background:
      return 10;
    case kThreadPriority_Normal:
      return 0;
    case kThreadPriority_Display:
      return -6;
    default:
      NOTREACHED() << "Unknown priority.";
      return 0;
  }
}

}  


void PlatformThread::SetName(const char* name) {
  ThreadIdNameManager::GetInstance()->SetName(CurrentId(), name);
  tracked_objects::ThreadData::InitializeThreadContext(name);

#if !defined(OS_NACL)
  
  
  
  
  if (PlatformThread::CurrentId() == getpid())
    return;

  
  
  
  
  
  int err = prctl(PR_SET_NAME, name);
  
  if (err < 0 && errno != EPERM)
    DPLOG(ERROR) << "prctl(PR_SET_NAME)";
#endif  
}


void PlatformThread::SetThreadPriority(PlatformThreadHandle handle,
                                       ThreadPriority priority) {
#if !defined(OS_NACL)
  if (priority == kThreadPriority_RealtimeAudio) {
    const struct sched_param kRealTimePrio = {8};
    if (pthread_setschedparam(pthread_self(), SCHED_RR, &kRealTimePrio) == 0) {
      
      return;
    }
  }

  
  
  
  
  
  
  DCHECK_NE(handle.id_, kInvalidThreadId);
  const int kNiceSetting = ThreadNiceValue(priority);
  const PlatformThreadId current_id = PlatformThread::CurrentId();
  if (setpriority(PRIO_PROCESS,
                  handle.id_ == current_id ? 0 : handle.id_,
                  kNiceSetting)) {
    DVPLOG(1) << "Failed to set nice value of thread (" << handle.id_ << ") to "
              << kNiceSetting;
  }
#endif  
}

void InitThreading() {}

void InitOnThread() {}

void TerminateOnThread() {}

size_t GetDefaultThreadStackSize(const pthread_attr_t& attributes) {
#if !defined(THREAD_SANITIZER)
  return 0;
#else
  
  
  return 2 * (1 << 23);  
#endif
}

}  
