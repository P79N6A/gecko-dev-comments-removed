









#ifndef WEBRTC_BASE_EVENT_H__
#define WEBRTC_BASE_EVENT_H__

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"  
#elif defined(WEBRTC_POSIX)
#include <pthread.h>
#else
#error "Must define either WEBRTC_WIN or WEBRTC_POSIX."
#endif

#include "webrtc/base/basictypes.h"
#include "webrtc/base/common.h"

namespace rtc {

class Event {
 public:
  Event(bool manual_reset, bool initially_signaled);
  ~Event();

  void Set();
  void Reset();
  bool Wait(int cms);

 private:
  bool is_manual_reset_;

#if defined(WEBRTC_WIN)
  bool is_initially_signaled_;
  HANDLE event_handle_;
#elif defined(WEBRTC_POSIX)
  bool event_status_;
  pthread_mutex_t event_mutex_;
  pthread_cond_t event_cond_;
#endif
};

}  

#endif  
