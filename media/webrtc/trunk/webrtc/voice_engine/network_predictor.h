









#ifndef WEBRTC_VOICE_ENGINE_NETWORK_PREDICTOR_H_
#define WEBRTC_VOICE_ENGINE_NETWORK_PREDICTOR_H_

#include "webrtc/base/exp_filter.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {

namespace voe {



class NetworkPredictor {
 public:
  explicit NetworkPredictor(Clock* clock);
  ~NetworkPredictor() {}

  
  uint8_t GetLossRate();

  
  
  
  void UpdatePacketLossRate(uint8_t loss_rate);

 private:
  Clock* clock_;
  int64_t last_loss_rate_update_time_ms_;

  
  scoped_ptr<rtc::ExpFilter> loss_rate_filter_;
};

}  
}  
#endif  
