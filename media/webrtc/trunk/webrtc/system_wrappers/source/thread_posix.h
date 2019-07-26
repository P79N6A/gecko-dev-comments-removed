









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_POSIX_H_

#include "webrtc/system_wrappers/interface/thread_wrapper.h"

#include <pthread.h>

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;

int ConvertToSystemPriority(ThreadPriority priority, int min_prio,
                            int max_prio);

class ThreadPosix : public ThreadWrapper {
 public:
  static ThreadWrapper* Create(ThreadRunFunction func, ThreadObj obj,
                               ThreadPriority prio, const char* thread_name);

  ThreadPosix(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio,
              const char* thread_name);
  ~ThreadPosix();

  
  virtual void SetNotAlive();
  virtual bool Start(unsigned int& id);
  
  virtual bool SetAffinity(const int* processor_numbers,
                           unsigned int amount_of_processors);
  virtual bool Stop();

  void Run();

 private:
  int Construct();

 private:
  ThreadRunFunction   run_function_;
  ThreadObj           obj_;

  
  CriticalSectionWrapper* crit_state_;  
  bool                    alive_;
  bool                    dead_;
  ThreadPriority          prio_;
  EventWrapper*           event_;

  
  char                    name_[kThreadMaxNameLength];
  bool                    set_thread_name_;

  
#if (defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID))
  pid_t                   pid_;
#endif
  pthread_attr_t          attr_;
  pthread_t               thread_;
};

} 

#endif  
