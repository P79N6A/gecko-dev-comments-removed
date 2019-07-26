









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_EVENT_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_EVENT_WRAPPER_H_

namespace webrtc {
enum EventTypeWrapper {
  kEventSignaled = 1,
  kEventError = 2,
  kEventTimeout = 3
};

#define WEBRTC_EVENT_10_SEC   10000
#define WEBRTC_EVENT_INFINITE 0xffffffff

class EventWrapper {
 public:
  
  static EventWrapper* Create();
  virtual ~EventWrapper() {}

  
  
  
  
  
  
  virtual bool Set() = 0;

  
  virtual bool Reset() = 0;

  
  
  
  
  
  
  
  virtual EventTypeWrapper Wait(unsigned long max_time) = 0;

  
  
  
  virtual bool StartTimer(bool periodic, unsigned long time) = 0;

  virtual bool StopTimer() = 0;

  
  
  
  
  
  static int KeyPressed();
};
} 

#endif  
