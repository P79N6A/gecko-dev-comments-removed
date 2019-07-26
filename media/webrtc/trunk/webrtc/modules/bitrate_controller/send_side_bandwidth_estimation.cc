









#include "modules/bitrate_controller/send_side_bandwidth_estimation.h"

#include <math.h>  

#include "system_wrappers/interface/trace.h"

namespace webrtc {

SendSideBandwidthEstimation::SendSideBandwidthEstimation()
    : critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      accumulate_lost_packets_Q8_(0),
      accumulate_expected_packets_(0),
      bitrate_(0),
      min_bitrate_configured_(0),
      max_bitrate_configured_(0),
      last_fraction_loss_(0),
      last_round_trip_time_(0),
      bwe_incoming_(0),
      time_last_increase_(0),
      time_last_decrease_(0) {
}

SendSideBandwidthEstimation::~SendSideBandwidthEstimation() {
    delete critsect_;
}

void SendSideBandwidthEstimation::SetSendBitrate(const uint32_t bitrate) {
  CriticalSectionScoped cs(critsect_);
  bitrate_ = bitrate;
}

void SendSideBandwidthEstimation::SetMinMaxBitrate(const uint32_t min_bitrate,
                                                   const uint32_t max_bitrate) {
  CriticalSectionScoped cs(critsect_);
  min_bitrate_configured_ = min_bitrate;
  if (max_bitrate == 0) {
    
    max_bitrate_configured_ = 1000000000;
  } else {
    max_bitrate_configured_ = max_bitrate;
  }
}

bool SendSideBandwidthEstimation::UpdateBandwidthEstimate(
    const uint32_t bandwidth,
    uint32_t* new_bitrate,
    uint8_t* fraction_lost,
    uint16_t* rtt) {
  *new_bitrate = 0;
  CriticalSectionScoped cs(critsect_);

  bwe_incoming_ = bandwidth;

  if (bitrate_ == 0) {
    
    return false;
  }
  if (bwe_incoming_ > 0 && bitrate_ > bwe_incoming_) {
    bitrate_ = bwe_incoming_;
    *new_bitrate = bitrate_;
    *fraction_lost = last_fraction_loss_;
    *rtt = last_round_trip_time_;
    return true;
  }
  return false;
}

bool SendSideBandwidthEstimation::UpdatePacketLoss(
    const int number_of_packets,
    const uint32_t rtt,
    const uint32_t now_ms,
    uint8_t* loss,
    uint32_t* new_bitrate) {
  CriticalSectionScoped cs(critsect_);

  if (bitrate_ == 0) {
    
    return false;
  }
  
  last_round_trip_time_ = rtt;

  
  if (number_of_packets > 0) {
    
    const int num_lost_packets_Q8 = *loss * number_of_packets;
    
    accumulate_lost_packets_Q8_ += num_lost_packets_Q8;
    accumulate_expected_packets_ += number_of_packets;

    
    if (accumulate_expected_packets_ >= kLimitNumPackets) {
      *loss = accumulate_lost_packets_Q8_ / accumulate_expected_packets_;

      
      accumulate_lost_packets_Q8_ = 0;
      accumulate_expected_packets_ = 0;
    } else {
      
      
      return false;
    }
  }
  
  last_fraction_loss_ = *loss;
  uint32_t bitrate = 0;
  if (!ShapeSimple(*loss, rtt, now_ms, &bitrate)) {
    
    return false;
  }
  bitrate_ = bitrate;
  *new_bitrate = bitrate;
  return true;
}

bool SendSideBandwidthEstimation::AvailableBandwidth(
    uint32_t* bandwidth) const {
  CriticalSectionScoped cs(critsect_);
  if (bitrate_ == 0) {
    return false;
  }
  *bandwidth = bitrate_;
  return true;
}





uint32_t SendSideBandwidthEstimation::CalcTFRCbps(uint16_t rtt, uint8_t loss) {
  if (rtt == 0 || loss == 0) {
    
    return 0;
  }
  double R = static_cast<double>(rtt) / 1000;  
  int b = 1;  
              
  double t_RTO = 4.0 * R;  
                           
  double p = static_cast<double>(loss) / 255;  
  double s = static_cast<double>(kAvgPacketSizeBytes);

  
  double X = s / (R * sqrt(2 * b * p / 3) +
      (t_RTO * (3 * sqrt(3 * b * p / 8) * p * (1 + 32 * p * p))));

  return (static_cast<uint32_t>(X * 8));  
}

bool SendSideBandwidthEstimation::ShapeSimple(const uint8_t loss,
                                              const uint32_t rtt,
                                              const uint32_t now_ms,
                                              uint32_t* bitrate) {
  uint32_t new_bitrate = 0;
  bool reducing = false;

  
  if (loss <= 5) {
    if ((now_ms - time_last_increase_) < kBWEIncreaseIntervalMs) {
      return false;
    }
    time_last_increase_ = now_ms;
  }
  
  if (loss > 26) {
    if ((now_ms - time_last_decrease_) < kBWEDecreaseIntervalMs + rtt) {
      return false;
    }
    time_last_decrease_ = now_ms;
  }

  if (loss > 5 && loss <= 26) {
    
    new_bitrate = bitrate_;
  } else if (loss > 26) {
    
    
    
    new_bitrate = static_cast<uint32_t>((bitrate_ *
        static_cast<double>(512 - loss)) / 512.0);
    reducing = true;
  } else {
    
    new_bitrate = static_cast<uint32_t>(bitrate_ * 1.08 + 0.5);

    
    
    new_bitrate += 1000;
  }
  if (reducing) {
    
    
    uint32_t tfrc_bitrate = CalcTFRCbps(rtt, loss);
    if (tfrc_bitrate > new_bitrate) {
      
      new_bitrate = tfrc_bitrate;
    }
  }
  if (bwe_incoming_ > 0 && new_bitrate > bwe_incoming_) {
    new_bitrate = bwe_incoming_;
  }
  if (new_bitrate > max_bitrate_configured_) {
    new_bitrate = max_bitrate_configured_;
  }
  if (new_bitrate < min_bitrate_configured_) {
    WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, -1,
                 "The configured min bitrate (%u kbps) is greater than the "
                 "estimated available bandwidth (%u kbps).\n",
                 min_bitrate_configured_ / 1000, new_bitrate / 1000);
    new_bitrate = min_bitrate_configured_;
  }
  *bitrate = new_bitrate;
  return true;
}
}  
