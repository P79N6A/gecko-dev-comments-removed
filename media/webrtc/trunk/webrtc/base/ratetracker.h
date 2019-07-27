









#ifndef WEBRTC_BASE_RATETRACKER_H_
#define WEBRTC_BASE_RATETRACKER_H_

#include <stdlib.h>
#include "webrtc/base/basictypes.h"

namespace rtc {


class RateTracker {
 public:
  RateTracker();
  virtual ~RateTracker() {}

  size_t total_units() const;
  size_t units_second();
  void Update(size_t units);

 protected:
  
  virtual uint32 Time() const;

 private:
  size_t total_units_;
  size_t units_second_;
  uint32 last_units_second_time_;
  size_t last_units_second_calc_;
};

}  

#endif  
