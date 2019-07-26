









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_REMOTE_RATE_CONTROL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_REMOTE_RATE_CONTROL_H_

#include "webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h"

namespace webrtc {

class RemoteRateControl {
 public:
  explicit RemoteRateControl(uint32_t min_bitrate_bps);
  ~RemoteRateControl() {}

  void Reset();

  
  
  bool ValidEstimate() const;

  
  
  
  
  bool TimeToReduceFurther(int64_t time_now,
                           unsigned int incoming_bitrate) const;

  int32_t SetConfiguredBitRates(uint32_t min_bit_rate, uint32_t max_bit_rate);
  uint32_t LatestEstimate() const;
  uint32_t UpdateBandwidthEstimate(int64_t now_ms);
  void SetRtt(unsigned int rtt);
  RateControlRegion Update(const RateControlInput* input, int64_t now_ms);

 private:
  uint32_t ChangeBitRate(uint32_t current_bit_rate,
                         uint32_t incoming_bit_rate,
                         double delay_factor,
                         int64_t now_ms);
  double RateIncreaseFactor(int64_t now_ms,
                            int64_t last_ms,
                            uint32_t reaction_time_ms,
                            double noise_var) const;
  void UpdateChangePeriod(int64_t now_ms);
  void UpdateMaxBitRateEstimate(float incoming_bit_rate_kbps);
  void ChangeState(const RateControlInput& input, int64_t now_ms);
  void ChangeState(RateControlState new_state);
  void ChangeRegion(RateControlRegion region);
  static void StateStr(RateControlState state, char* str);
  static void StateStr(BandwidthUsage state, char* str);

  uint32_t min_configured_bit_rate_;
  uint32_t max_configured_bit_rate_;
  uint32_t current_bit_rate_;
  uint32_t max_hold_rate_;
  float avg_max_bit_rate_;
  float var_max_bit_rate_;
  RateControlState rate_control_state_;
  RateControlState came_from_state_;
  RateControlRegion rate_control_region_;
  int64_t last_bit_rate_change_;
  RateControlInput current_input_;
  bool updated_;
  int64_t time_first_incoming_estimate_;
  bool initialized_bit_rate_;
  float avg_change_period_;
  int64_t last_change_ms_;
  float beta_;
  unsigned int rtt_;
};
}  

#endif 
