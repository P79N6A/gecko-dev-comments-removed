











#ifndef WEBRTC_MODULES_BITRATE_CONTROLLER_SEND_SIDE_BANDWIDTH_ESTIMATION_H_
#define WEBRTC_MODULES_BITRATE_CONTROLLER_SEND_SIDE_BANDWIDTH_ESTIMATION_H_

#include <deque>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {
class SendSideBandwidthEstimation {
 public:
  SendSideBandwidthEstimation();
  virtual ~SendSideBandwidthEstimation();

  void CurrentEstimate(uint32_t* bitrate, uint8_t* loss, uint32_t* rtt) const;

  
  void UpdateEstimate(uint32_t now_ms);

  
  void UpdateReceiverEstimate(uint32_t bandwidth);

  
  void UpdateReceiverBlock(uint8_t fraction_loss,
                           uint32_t rtt,
                           int number_of_packets,
                           uint32_t now_ms);

  void SetSendBitrate(uint32_t bitrate);
  void SetMinMaxBitrate(uint32_t min_bitrate, uint32_t max_bitrate);
  void SetMinBitrate(uint32_t min_bitrate);

 private:
  enum UmaState { kNoUpdate, kFirstDone, kDone };

  bool IsInStartPhase(int64_t now_ms) const;

  void UpdateUmaStats(int64_t now_ms, int rtt, int lost_packets);

  
  
  uint32_t CapBitrateToThresholds(uint32_t bitrate);

  
  
  
  void UpdateMinHistory(uint32_t now_ms);

  std::deque<std::pair<uint32_t, uint32_t> > min_bitrate_history_;

  
  int accumulate_lost_packets_Q8_;
  int accumulate_expected_packets_;

  uint32_t bitrate_;
  uint32_t min_bitrate_configured_;
  uint32_t max_bitrate_configured_;

  uint32_t time_last_receiver_block_ms_;
  uint8_t last_fraction_loss_;
  uint16_t last_round_trip_time_ms_;

  uint32_t bwe_incoming_;
  uint32_t time_last_decrease_ms_;
  int64_t first_report_time_ms_;
  int initially_lost_packets_;
  int bitrate_at_2_seconds_kbps_;
  UmaState uma_update_state_;
};
}  
#endif
