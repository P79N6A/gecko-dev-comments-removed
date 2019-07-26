











































#include "webrtc/system_wrappers/source/thread_posix.h"

#include <algorithm>

#include <assert.h>
#include <errno.h>
#include <string.h>  
#include <time.h>    
#include <unistd.h>
#ifdef WEBRTC_LINUX
#include <sys/types.h>
#include <sched.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
#endif

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

int ConvertToSystemPriority(ThreadPriority priority, int min_prio,
                            int max_prio) {
  assert(max_prio - min_prio > 2);
  const int top_prio = max_prio - 1;
  const int low_prio = min_prio + 1;

  switch (priority) {
    case kLowPriority:
      return low_prio;
    case kNormalPriority:
      
      
      return (low_prio + top_prio - 1) / 2;
    case kHighPriority:
      return std::max(top_prio - 2, low_prio);
    case kHighestPriority:
      return std::max(top_prio - 1, low_prio);
    case kRealtimePriority:
      return top_prio;
  }
  assert(false);
  return low_prio;
}

extern "C"
{
  static void* StartThread(void* lp_parameter) {
    static_cast<ThreadPosix*>(lp_parameter)->Run();
    return 0;
  }
}

ThreadWrapper* ThreadPosix::Create(ThreadRunFunction func,
                                   ThreadObj obj,
                                   ThreadPriority prio,
                                   const char* thread_name) {
  ThreadPosix* ptr = new ThreadPosix(func, obj, prio, thread_name);
  if (!ptr) {
    return NULL;
  }
  const int error = ptr->Construct();
  if (error) {
    delete ptr;
    return NULL;
  }
  return ptr;
}

ThreadPosix::ThreadPosix(ThreadRunFunction func, ThreadObj obj,
                         ThreadPriority prio, const char* thread_name)
    : run_function_(func),
      obj_(obj),
      crit_state_(CriticalSectionWrapper::CreateCriticalSection()),
      alive_(false),
      dead_(true),
      prio_(prio),
      event_(EventWrapper::Create()),
      name_(),
      set_thread_name_(false),
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID) || defined(WEBRTC_GONK))
      pid_(-1),
#endif
      attr_(),
      thread_(0) {
  if (thread_name != NULL) {
    set_thread_name_ = true;
    strncpy(name_, thread_name, kThreadMaxNameLength);
    name_[kThreadMaxNameLength - 1] = '\0';
  }
}

uint32_t ThreadWrapper::GetThreadId() {
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_LINUX) || defined(WEBRTC_GONK)
  return static_cast<uint32_t>(syscall(__NR_gettid));
#elif defined(WEBRTC_MAC) || defined(WEBRTC_IOS)
  return pthread_mach_thread_np(pthread_self());
#else
  return reinterpret_cast<uint32_t>(pthread_self());
#endif
}

int ThreadPosix::Construct() {
  int result = 0;
#if !defined(WEBRTC_ANDROID) && !defined(WEBRTC_GONK)
  
  result = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (result != 0) {
    return -1;
  }
  result = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  if (result != 0) {
    return -1;
  }
#endif
  result = pthread_attr_init(&attr_);
  if (result != 0) {
    return -1;
  }
  return 0;
}

ThreadPosix::~ThreadPosix() {
  pthread_attr_destroy(&attr_);
  delete event_;
  delete crit_state_;
}

#define HAS_THREAD_ID !defined(WEBRTC_IOS) && !defined(WEBRTC_MAC)

bool ThreadPosix::Start(unsigned int& thread_id)
{
  if (!run_function_) {
    return false;
  }
  int result = pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_DETACHED);
  
  result |= pthread_attr_setstacksize(&attr_, 1024 * 1024);
#ifdef WEBRTC_THREAD_RR
  const int policy = SCHED_RR;
#else
  const int policy = SCHED_FIFO;
#endif
  event_->Reset();
  
  
  
  
  
  result |= pthread_create(&thread_, &attr_, &StartThread, this);
  if (result != 0) {
    return false;
  }

  
  
  if (kEventSignaled != event_->Wait(WEBRTC_EVENT_10_SEC)) {
    WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                 "posix thread event never triggered");

    
    run_function_ = NULL;
    return true;
  }

#if HAS_THREAD_ID
  thread_id = static_cast<unsigned int>(thread_);
#endif
  sched_param param;

  const int min_prio = sched_get_priority_min(policy);
  const int max_prio = sched_get_priority_max(policy);

  if ((min_prio == EINVAL) || (max_prio == EINVAL)) {
    WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                 "unable to retreive min or max priority for threads");
    return true;
  }
  if (max_prio - min_prio <= 2) {
    
    return true;
  }
  param.sched_priority = ConvertToSystemPriority(prio_, min_prio, max_prio);
  result = pthread_setschedparam(thread_, policy, &param);
  if (result == EINVAL) {
    WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                 "unable to set thread priority");
  }
  return true;
}



#if (defined(WEBRTC_LINUX) && (!defined(WEBRTC_ANDROID)) && (!defined(WEBRTC_GONK)))
bool ThreadPosix::SetAffinity(const int* processor_numbers,
                              const unsigned int amount_of_processors) {
  if (!processor_numbers || (amount_of_processors == 0)) {
    return false;
  }
  cpu_set_t mask;
  CPU_ZERO(&mask);

  for (unsigned int processor = 0;
       processor < amount_of_processors;
       ++processor) {
    CPU_SET(processor_numbers[processor], &mask);
  }
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_GONK)
  
  const int result = syscall(__NR_sched_setaffinity,
                             pid_,
                             sizeof(mask),
                             &mask);
#else
  
  const int result = sched_setaffinity(pid_,
                                       sizeof(mask),
                                       &mask);
#endif
  if (result != 0) {
    return false;
  }
  return true;
}

#else



bool ThreadPosix::SetAffinity(const int* , const unsigned int) {
  return false;
}
#endif

void ThreadPosix::SetNotAlive() {
  CriticalSectionScoped cs(crit_state_);
  alive_ = false;
}

bool ThreadPosix::Stop() {
  bool dead = false;
  {
    CriticalSectionScoped cs(crit_state_);
    alive_ = false;
    dead = dead_;
  }

  
  
  for (int i = 0; i < 1000 && !dead; i++) {
    timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 10 * 1000 * 1000;
    nanosleep(&t, NULL);
    {
      CriticalSectionScoped cs(crit_state_);
      dead = dead_;
    }
  }
  if (dead) {
    return true;
  } else {
    return false;
  }
}

void ThreadPosix::Run() {
  {
    CriticalSectionScoped cs(crit_state_);
    alive_ = true;
    dead_  = false;
  }
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID) || defined(WEBRTC_GONK))
  pid_ = GetThreadId();
#endif
  
  event_->Set();

  if (set_thread_name_) {
#ifdef WEBRTC_LINUX
    prctl(PR_SET_NAME, (unsigned long)name_, 0, 0, 0);
#endif
    WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                 "Thread with name:%s started ", name_);
  } else {
    WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                 "Thread without name started");
  }
  bool alive = true;
  do {
    if (run_function_) {
      if (!run_function_(obj_)) {
        alive = false;
      }
    } else {
      alive = false;
    }
    {
      CriticalSectionScoped cs(crit_state_);
      if (!alive) {
        alive_ = false;
      }
      alive = alive_;
    }
  } while (alive);

  if (set_thread_name_) {
    
    
    
    if (strcmp(name_, "Trace")) {
      WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                   "Thread with name:%s stopped", name_);
    }
  } else {
    WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                 "Thread without name stopped");
  }
  {
    CriticalSectionScoped cs(crit_state_);
    dead_ = true;
  }
}

} 
