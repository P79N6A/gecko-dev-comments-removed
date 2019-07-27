









#ifndef WEBRTC_EXPERIMENTS_H_
#define WEBRTC_EXPERIMENTS_H_

#include "webrtc/typedefs.h"

namespace webrtc {
struct RemoteBitrateEstimatorMinRate {
  RemoteBitrateEstimatorMinRate() : min_rate(30000) {}
  RemoteBitrateEstimatorMinRate(uint32_t min_rate) : min_rate(min_rate) {}

  uint32_t min_rate;
};

struct AimdRemoteRateControl {
  AimdRemoteRateControl() : enabled(false) {}
  explicit AimdRemoteRateControl(bool set_enabled)
    : enabled(set_enabled) {}
  virtual ~AimdRemoteRateControl() {}

  const bool enabled;
};
}  
#endif  
