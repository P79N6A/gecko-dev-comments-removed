









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_

#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {


const uint32_t kNtpJan1970 = 2208988800UL;


const double kMagicNtpFractionalUnit = 4.294967296E+9;


class Clock {
 public:
  virtual ~Clock() {}

  
  
  virtual int64_t TimeInMilliseconds() const = 0;

  
  
  virtual int64_t TimeInMicroseconds() const = 0;

  
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) const = 0;

  
  virtual int64_t CurrentNtpInMilliseconds() const = 0;

  
  static int64_t NtpToMs(uint32_t seconds, uint32_t fractions);

  
  static Clock* GetRealTimeClock();
};

class SimulatedClock : public Clock {
 public:
  explicit SimulatedClock(int64_t initial_time_us);

  virtual ~SimulatedClock();

  
  
  virtual int64_t TimeInMilliseconds() const OVERRIDE;

  
  
  virtual int64_t TimeInMicroseconds() const OVERRIDE;

  
  virtual void CurrentNtp(uint32_t& seconds,
                          uint32_t& fractions) const OVERRIDE;

  
  virtual int64_t CurrentNtpInMilliseconds() const OVERRIDE;

  
  
  void AdvanceTimeMilliseconds(int64_t milliseconds);
  void AdvanceTimeMicroseconds(int64_t microseconds);

 private:
  int64_t time_us_;
  scoped_ptr<RWLockWrapper> lock_;
};

};  

#endif  
