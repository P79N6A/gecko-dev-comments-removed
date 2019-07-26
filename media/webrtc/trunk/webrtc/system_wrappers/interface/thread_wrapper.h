














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

  
  
  
  
  
  
  
  
  static ThreadWrapper* CreateThread(ThreadRunFunction func = 0,
                                     ThreadObj obj = 0,
                                     ThreadPriority prio = kNormalPriority,
                                     const char* thread_name = 0);

  
  static uint32_t GetThreadId();

  
  
  virtual void SetNotAlive() = 0;

  
  
  
  
  
  
  virtual bool Start(unsigned int& id) = 0;

  
  
  
  
  
  virtual bool SetAffinity(const int* processor_numbers,
                           const unsigned int amount_of_processors) {
    return false;
  }

  
  
  
  
  virtual bool Stop() = 0;
};

} 

#endif
