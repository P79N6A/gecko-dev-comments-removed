









#include "webrtc/modules/remote_bitrate_estimator/rate_statistics.h"

namespace webrtc {

RateStatistics::RateStatistics(uint32_t window_size_ms, float scale)
    : num_buckets_(window_size_ms + 1),  
      buckets_(new uint32_t[num_buckets_]()),
      accumulated_count_(0),
      oldest_time_(0),
      oldest_index_(0),
      scale_(scale / (num_buckets_ - 1)) {
}

RateStatistics::~RateStatistics() {
}

void RateStatistics::Reset() {
  accumulated_count_ = 0;
  oldest_time_ = 0;
  oldest_index_ = 0;
  for (int i = 0; i < num_buckets_; i++) {
    buckets_[i] = 0;
  }
}

void RateStatistics::Update(uint32_t count, int64_t now_ms) {
  if (now_ms < oldest_time_) {
    
    return;
  }

  EraseOld(now_ms);

  int now_offset = static_cast<int>(now_ms - oldest_time_);
  assert(now_offset < num_buckets_);
  int index = oldest_index_ + now_offset;
  if (index >= num_buckets_) {
    index -= num_buckets_;
  }
  buckets_[index] += count;
  accumulated_count_ += count;
}

uint32_t RateStatistics::Rate(int64_t now_ms) {
  EraseOld(now_ms);
  return static_cast<uint32_t>(accumulated_count_ * scale_ + 0.5f);
}

void RateStatistics::EraseOld(int64_t now_ms) {
  int64_t new_oldest_time = now_ms - num_buckets_ + 1;
  if (new_oldest_time <= oldest_time_) {
    return;
  }

  while (oldest_time_ < new_oldest_time) {
    uint32_t count_in_oldest_bucket = buckets_[oldest_index_];
    assert(accumulated_count_ >= count_in_oldest_bucket);
    accumulated_count_ -= count_in_oldest_bucket;
    buckets_[oldest_index_] = 0;
    if (++oldest_index_ >= num_buckets_) {
      oldest_index_ = 0;
    }
    ++oldest_time_;
    if (accumulated_count_ == 0) {
      
      
      break;
    }
  }
  oldest_time_ = new_oldest_time;
}

}  
