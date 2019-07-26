









#include "webrtc/modules/remote_bitrate_estimator/bitrate_estimator.h"

namespace webrtc {

const float kBitrateAverageWindowMs = 500.0f;

BitRateStats::BitRateStats()
    : data_samples_(),
      accumulated_bytes_(0) {
}

BitRateStats::~BitRateStats() {
  Init();
}

void BitRateStats::Init() {
  accumulated_bytes_ = 0;
  while (data_samples_.size() > 0) {
    delete data_samples_.front();
    data_samples_.pop_front();
  }
}

void BitRateStats::Update(uint32_t packet_size_bytes, int64_t now_ms) {
  
  
  data_samples_.push_back(new DataTimeSizeTuple(packet_size_bytes, now_ms));
  accumulated_bytes_ += packet_size_bytes;
  EraseOld(now_ms);
}

void BitRateStats::EraseOld(int64_t now_ms) {
  while (data_samples_.size() > 0) {
    if (now_ms - data_samples_.front()->time_complete_ms >
        kBitrateAverageWindowMs) {
      
      accumulated_bytes_ -= data_samples_.front()->size_bytes;
      delete data_samples_.front();
      data_samples_.pop_front();
    } else {
      break;
    }
  }
}

uint32_t BitRateStats::BitRate(int64_t now_ms) {
  
  
  EraseOld(now_ms);
  return static_cast<uint32_t>(accumulated_bytes_ * 8.0f * 1000.0f /
                     kBitrateAverageWindowMs + 0.5f);
}
}  
