









#include "webrtc/modules/bitrate_controller/send_side_bandwidth_estimation.h"

#include <cmath>

#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/metrics.h"

namespace webrtc {
namespace {
enum { kBweIncreaseIntervalMs = 1000 };
enum { kBweDecreaseIntervalMs = 300 };
enum { kLimitNumPackets = 20 };
enum { kAvgPacketSizeBytes = 1000 };
enum { kStartPhaseMs = 2000 };
enum { kBweConverganceTimeMs = 20000 };



uint32_t CalcTfrcBps(uint16_t rtt, uint8_t loss) {
  if (rtt == 0 || loss == 0) {
    
    return 0;
  }
  double R = static_cast<double>(rtt) / 1000;  
  int b = 1;  
              
  double t_RTO = 4.0 * R;  
                           
  double p = static_cast<double>(loss) / 255;  
  double s = static_cast<double>(kAvgPacketSizeBytes);

  
  double X =
      s / (R * std::sqrt(2 * b * p / 3) +
           (t_RTO * (3 * std::sqrt(3 * b * p / 8) * p * (1 + 32 * p * p))));

  
  return (static_cast<uint32_t>(X * 8));
}
}

SendSideBandwidthEstimation::SendSideBandwidthEstimation()
    : accumulate_lost_packets_Q8_(0),
      accumulate_expected_packets_(0),
      bitrate_(0),
      min_bitrate_configured_(0),
      max_bitrate_configured_(0),
      time_last_receiver_block_ms_(0),
      last_fraction_loss_(0),
      last_round_trip_time_ms_(0),
      bwe_incoming_(0),
      time_last_decrease_ms_(0),
      first_report_time_ms_(-1),
      initially_lost_packets_(0),
      bitrate_at_2_seconds_kbps_(0),
      uma_update_state_(kNoUpdate) {
}

SendSideBandwidthEstimation::~SendSideBandwidthEstimation() {}

void SendSideBandwidthEstimation::SetSendBitrate(uint32_t bitrate) {
  bitrate_ = bitrate;

  
  
  min_bitrate_history_.clear();
}

void SendSideBandwidthEstimation::SetMinMaxBitrate(uint32_t min_bitrate,
                                                   uint32_t max_bitrate) {
  min_bitrate_configured_ = min_bitrate;
  max_bitrate_configured_ = max_bitrate;
}

void SendSideBandwidthEstimation::SetMinBitrate(uint32_t min_bitrate) {
  min_bitrate_configured_ = min_bitrate;
}

void SendSideBandwidthEstimation::CurrentEstimate(uint32_t* bitrate,
                                                  uint8_t* loss,
                                                  uint32_t* rtt) const {
  *bitrate = bitrate_;
  *loss = last_fraction_loss_;
  *rtt = last_round_trip_time_ms_;
}

void SendSideBandwidthEstimation::UpdateReceiverEstimate(uint32_t bandwidth) {
  bwe_incoming_ = bandwidth;
  bitrate_ = CapBitrateToThresholds(bitrate_);
}

void SendSideBandwidthEstimation::UpdateReceiverBlock(uint8_t fraction_loss,
                                                      uint32_t rtt,
                                                      int number_of_packets,
                                                      uint32_t now_ms) {
  
  last_round_trip_time_ms_ = rtt;

  
  if (number_of_packets > 0) {
    
    const int num_lost_packets_Q8 = fraction_loss * number_of_packets;
    
    accumulate_lost_packets_Q8_ += num_lost_packets_Q8;
    accumulate_expected_packets_ += number_of_packets;

    
    if (accumulate_expected_packets_ >= kLimitNumPackets) {
      last_fraction_loss_ =
          accumulate_lost_packets_Q8_ / accumulate_expected_packets_;

      
      accumulate_lost_packets_Q8_ = 0;
      accumulate_expected_packets_ = 0;
    } else {
      
      return;
    }
  }
  time_last_receiver_block_ms_ = now_ms;
  UpdateEstimate(now_ms);

  if (first_report_time_ms_ == -1) {
    first_report_time_ms_ = now_ms;
  } else {
    UpdateUmaStats(now_ms, rtt, (fraction_loss * number_of_packets) >> 8);
  }
}

void SendSideBandwidthEstimation::UpdateUmaStats(int64_t now_ms,
                                                 int rtt,
                                                 int lost_packets) {
  if (IsInStartPhase(now_ms)) {
    initially_lost_packets_ += lost_packets;
  } else if (uma_update_state_ == kNoUpdate) {
    uma_update_state_ = kFirstDone;
    bitrate_at_2_seconds_kbps_ = (bitrate_ + 500) / 1000;
    RTC_HISTOGRAM_COUNTS(
        "WebRTC.BWE.InitiallyLostPackets", initially_lost_packets_, 0, 100, 50);
    RTC_HISTOGRAM_COUNTS("WebRTC.BWE.InitialRtt", rtt, 0, 2000, 50);
    RTC_HISTOGRAM_COUNTS("WebRTC.BWE.InitialBandwidthEstimate",
                         bitrate_at_2_seconds_kbps_,
                         0,
                         2000,
                         50);
  } else if (uma_update_state_ == kFirstDone &&
             now_ms - first_report_time_ms_ >= kBweConverganceTimeMs) {
    uma_update_state_ = kDone;
    int bitrate_diff_kbps = std::max(
        bitrate_at_2_seconds_kbps_ - static_cast<int>((bitrate_ + 500) / 1000),
        0);
    RTC_HISTOGRAM_COUNTS(
        "WebRTC.BWE.InitialVsConvergedDiff", bitrate_diff_kbps, 0, 2000, 50);
  }
}

void SendSideBandwidthEstimation::UpdateEstimate(uint32_t now_ms) {
  
  
  if (last_fraction_loss_ == 0 && IsInStartPhase(now_ms) &&
      bwe_incoming_ > bitrate_) {
    bitrate_ = CapBitrateToThresholds(bwe_incoming_);
    min_bitrate_history_.clear();
    min_bitrate_history_.push_back(std::make_pair(now_ms, bitrate_));
    return;
  }
  UpdateMinHistory(now_ms);
  
  if (time_last_receiver_block_ms_ != 0) {
    if (last_fraction_loss_ <= 5) {
      
      
      
      
      
      
      
      
      
      bitrate_ = static_cast<uint32_t>(
          min_bitrate_history_.front().second * 1.08 + 0.5);

      
      
      
      bitrate_ += 1000;

    } else if (last_fraction_loss_ <= 26) {
      

    } else {
      
      
      if ((now_ms - time_last_decrease_ms_) >=
          static_cast<uint32_t>(kBweDecreaseIntervalMs +
                                last_round_trip_time_ms_)) {
        time_last_decrease_ms_ = now_ms;

        
        
        
        bitrate_ = static_cast<uint32_t>(
            (bitrate_ * static_cast<double>(512 - last_fraction_loss_)) /
            512.0);

        
        
        bitrate_ = std::max(
            bitrate_,
            CalcTfrcBps(last_round_trip_time_ms_, last_fraction_loss_));
      }
    }
  }
  bitrate_ = CapBitrateToThresholds(bitrate_);
}

bool SendSideBandwidthEstimation::IsInStartPhase(int64_t now_ms) const {
  return first_report_time_ms_ == -1 ||
         now_ms - first_report_time_ms_ < kStartPhaseMs;
}

void SendSideBandwidthEstimation::UpdateMinHistory(uint32_t now_ms) {
  
  
  
  while (!min_bitrate_history_.empty() &&
         now_ms - min_bitrate_history_.front().first + 1 >
             kBweIncreaseIntervalMs) {
    min_bitrate_history_.pop_front();
  }

  
  
  while (!min_bitrate_history_.empty() &&
         bitrate_ <= min_bitrate_history_.back().second) {
    min_bitrate_history_.pop_back();
  }

  min_bitrate_history_.push_back(std::make_pair(now_ms, bitrate_));
}

uint32_t SendSideBandwidthEstimation::CapBitrateToThresholds(uint32_t bitrate) {
  if (bwe_incoming_ > 0 && bitrate > bwe_incoming_) {
    bitrate = bwe_incoming_;
  }
  if (bitrate > max_bitrate_configured_) {
    bitrate = max_bitrate_configured_;
  }
  if (bitrate < min_bitrate_configured_) {
    LOG(LS_WARNING) << "Estimated available bandwidth " << bitrate / 1000
                    << " kbps is below configured min bitrate "
                    << min_bitrate_configured_ / 1000 << " kbps.";
    bitrate = min_bitrate_configured_;
  }
  return bitrate;
}

}  
