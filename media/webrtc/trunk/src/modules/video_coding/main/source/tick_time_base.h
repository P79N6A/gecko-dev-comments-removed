









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_TICK_TIME_BASE_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_TICK_TIME_BASE_H_

#include "system_wrappers/interface/tick_util.h"

namespace webrtc {


class TickTimeBase {
 public:
  virtual ~TickTimeBase() {}

  
  virtual int64_t MillisecondTimestamp() const {
    return TickTime::MillisecondTimestamp();
  }

  
  virtual int64_t MicrosecondTimestamp() const {
    return TickTime::MicrosecondTimestamp();
  }
};

}  

#endif  
