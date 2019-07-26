









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_WIN_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_EVENT_WIN_H_

#include <windows.h>

#include "webrtc/system_wrappers/interface/event_wrapper.h"

#include "webrtc/typedefs.h"

namespace webrtc {

class EventWindows : public EventWrapper {
 public:
  EventWindows();
  virtual ~EventWindows();

  virtual EventTypeWrapper Wait(unsigned long max_time);
  virtual bool Set();
  virtual bool Reset();

  virtual bool StartTimer(bool periodic, unsigned long time);
  virtual bool StopTimer();

 private:
  HANDLE  event_;
  WebRtc_UWord32 timerID_;
};

}  

#endif  
