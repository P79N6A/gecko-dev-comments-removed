









#include "webrtc/base/timing.h"
#include "webrtc/base/timeutils.h"

#if defined(WEBRTC_POSIX)
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <mach/mach.h>
#include <mach/clock.h>
#endif
#elif defined(WEBRTC_WIN)
#include <sys/timeb.h>
#include "webrtc/base/win32.h"
#endif

namespace rtc {

Timing::Timing() {
#if defined(WEBRTC_WIN)
  
  
  
  
  
  timer_handle_ = CreateWaitableTimer(NULL,     
                                      FALSE,    
                                      NULL);    
#endif
}

Timing::~Timing() {
#if defined(WEBRTC_WIN)
  if (timer_handle_ != NULL)
    CloseHandle(timer_handle_);
#endif
}


double Timing::WallTimeNow() {
#if defined(WEBRTC_POSIX)
  struct timeval time;
  gettimeofday(&time, NULL);
  
  return (static_cast<double>(time.tv_sec) +
          static_cast<double>(time.tv_usec) * 1.0e-6);

#elif defined(WEBRTC_WIN)
  struct _timeb time;
  _ftime(&time);
  
  return (static_cast<double>(time.time) +
          static_cast<double>(time.millitm) * 1.0e-3);
#endif
}

double Timing::TimerNow() {
  return (static_cast<double>(TimeNanos()) / kNumNanosecsPerSec);
}

double Timing::BusyWait(double period) {
  double start_time = TimerNow();
  while (TimerNow() - start_time < period) {
  }
  return TimerNow() - start_time;
}

double Timing::IdleWait(double period) {
  double start_time = TimerNow();

#if defined(WEBRTC_POSIX)
  double sec_int, sec_frac = modf(period, &sec_int);
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(sec_int);
  ts.tv_nsec = static_cast<long>(sec_frac * 1.0e9);  

  
  

  
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
  }

#elif defined(WEBRTC_WIN)
  if (timer_handle_ != NULL) {
    LARGE_INTEGER due_time;

    
    due_time.QuadPart = -LONGLONG(period * 1.0e7);

    SetWaitableTimer(timer_handle_, &due_time, 0, NULL, NULL, TRUE);
    WaitForSingleObject(timer_handle_, INFINITE);
  } else {
    
    
    Sleep(DWORD(period * 1.0e3));
  }
#endif

  return TimerNow() - start_time;
}

}  
