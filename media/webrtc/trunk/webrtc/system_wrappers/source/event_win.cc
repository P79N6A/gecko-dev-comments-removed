









#include "webrtc/system_wrappers/source/event_win.h"

#include "Mmsystem.h"

namespace webrtc {

EventWindows::EventWindows()
    : event_(::CreateEvent(NULL,    
                           FALSE,   
                           FALSE,   
                           NULL)),  
    timerID_(NULL) {
}

EventWindows::~EventWindows() {
  CloseHandle(event_);
}

bool EventWindows::Set() {
  
  return SetEvent(event_) == 1 ? true : false;
}

bool EventWindows::Reset() {
  return ResetEvent(event_) == 1 ? true : false;
}

EventTypeWrapper EventWindows::Wait(unsigned long max_time) {
  unsigned long res = WaitForSingleObject(event_, max_time);
  switch (res) {
    case WAIT_OBJECT_0:
      return kEventSignaled;
    case WAIT_TIMEOUT:
      return kEventTimeout;
    default:
      return kEventError;
  }
}

bool EventWindows::StartTimer(bool periodic, unsigned long time) {
  if (timerID_ != NULL) {
    timeKillEvent(timerID_);
    timerID_ = NULL;
  }
  if (periodic) {
    timerID_ = timeSetEvent(time, 0, (LPTIMECALLBACK)HANDLE(event_), 0,
                            TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE);
  } else {
    timerID_ = timeSetEvent(time, 0, (LPTIMECALLBACK)HANDLE(event_), 0,
                            TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
  }

  if (timerID_ == NULL) {
    return false;
  }
  return true;
}

bool EventWindows::StopTimer() {
  timeKillEvent(timerID_);
  timerID_ = NULL;
  return true;
}

}  
