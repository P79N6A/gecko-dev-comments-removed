









#include "webrtc/voice_engine/network_predictor.h"

namespace webrtc {
namespace voe {

NetworkPredictor::NetworkPredictor(Clock* clock)
    : clock_(clock),
      last_loss_rate_update_time_ms_(clock_->TimeInMilliseconds()),
      loss_rate_filter_(new rtc::ExpFilter(0.9999f)) {
}

void NetworkPredictor::UpdatePacketLossRate(uint8_t loss_rate) {
  int64_t now_ms = clock_->TimeInMilliseconds();
  
  loss_rate_filter_->Apply(
      static_cast<float>(now_ms - last_loss_rate_update_time_ms_),
      static_cast<float>(loss_rate));
  last_loss_rate_update_time_ms_ = now_ms;
}

uint8_t NetworkPredictor::GetLossRate() {
  float value = loss_rate_filter_->filtered();
  return (value == rtc::ExpFilter::kValueUndefined) ? 0 :
      static_cast<uint8_t>(value + 0.5);
}
}  
}  
