














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_THREAD_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_THREAD_WRAPPER_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {



#define ThreadObj void*




typedef bool(*ThreadRunFunction)(ThreadObj);

enum ThreadPriority {
  kLowPriority = 1,
  kNormalPriority = 2,
  kHighPriority = 3,
  kHighestPriority = 4,
  kRealtimePriority = 5
};

class ThreadWrapper {
 public:
  enum {kThreadMaxNameLength = 64};

  virtual ~ThreadWrapper() {};

  
  
  
  
  
  
  
  
  static ThreadWrapper* CreateThread(ThreadRunFunction func,
                                     ThreadObj obj,
                                     ThreadPriority prio = kNormalPriority,
                                     const char* thread_name = NULL);

  static ThreadWrapper* CreateUIThread(ThreadRunFunction func,
                                       ThreadObj obj,
                                       ThreadPriority prio = kNormalPriority,
                                       const char* thread_name = NULL);

  
  static uint32_t GetThreadId();

  
  
  virtual void SetNotAlive() = 0;

  
  
  
  
  
  
  virtual bool Start(unsigned int& id) = 0;

  
  
  
  
  
  virtual bool SetAffinity(const int* processor_numbers,
                           const unsigned int amount_of_processors);

  
  
  
  
  virtual bool Stop() = 0;

  
  
  virtual bool RequestCallbackTimer(unsigned int milliseconds);
};

}  

#endif
