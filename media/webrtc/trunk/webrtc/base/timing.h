









#ifndef WEBRTC_BASE_TIMING_H_
#define WEBRTC_BASE_TIMING_H_

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

namespace rtc {

class Timing {
 public:
  Timing();
  virtual ~Timing();

  
  
  
  static double WallTimeNow();

  
  
  
  
  
  
  virtual double TimerNow();

  
  
  
  double BusyWait(double period);

  
  
  
  
  
  
  
  double IdleWait(double period);

 private:
#if defined(WEBRTC_WIN)
  HANDLE timer_handle_;
#endif
};

}  

#endif 
