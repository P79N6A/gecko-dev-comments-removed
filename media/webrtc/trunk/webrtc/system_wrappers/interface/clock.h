









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_

#include "webrtc/typedefs.h"

namespace webrtc {


const uint32_t kNtpJan1970 = 2208988800UL;


const double kMagicNtpFractionalUnit = 4.294967296E+9;


class Clock {
 public:
  virtual ~Clock() {}

  
  
  virtual int64_t TimeInMilliseconds() = 0;

  
  
  virtual int64_t TimeInMicroseconds() = 0;

  
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) = 0;

  
  virtual int64_t CurrentNtpInMilliseconds() = 0;

  
  static int64_t NtpToMs(uint32_t seconds, uint32_t fractions);

  
  static Clock* GetRealTimeClock();
};

class SimulatedClock : public Clock {
 public:
  explicit SimulatedClock(int64_t initial_time_us);

  virtual ~SimulatedClock() {}

  
  
  virtual int64_t TimeInMilliseconds() OVERRIDE;

  
  
  virtual int64_t TimeInMicroseconds() OVERRIDE;

  
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) OVERRIDE;

  
  virtual int64_t CurrentNtpInMilliseconds() OVERRIDE;

  
  
  void AdvanceTimeMilliseconds(int64_t milliseconds);
  void AdvanceTimeMicroseconds(int64_t microseconds);

 private:
  int64_t time_us_;
};

};  

#endif  
