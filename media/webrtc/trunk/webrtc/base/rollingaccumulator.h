









#ifndef WEBRTC_BASE_ROLLINGACCUMULATOR_H_
#define WEBRTC_BASE_ROLLINGACCUMULATOR_H_

#include <vector>

#include "webrtc/base/common.h"

namespace rtc {





template<typename T>
class RollingAccumulator {
 public:
  explicit RollingAccumulator(size_t max_count)
    : samples_(max_count) {
    Reset();
  }
  ~RollingAccumulator() {
  }

  size_t max_count() const {
    return samples_.size();
  }

  size_t count() const {
    return count_;
  }

  void Reset() {
    count_ = 0U;
    next_index_ = 0U;
    sum_ = 0.0;
    sum_2_ = 0.0;
    max_ = T();
    max_stale_ = false;
    min_ = T();
    min_stale_ = false;
  }

  void AddSample(T sample) {
    if (count_ == max_count()) {
      
      T sample_to_remove = samples_[next_index_];
      sum_ -= sample_to_remove;
      sum_2_ -= sample_to_remove * sample_to_remove;
      if (sample_to_remove >= max_) {
        max_stale_ = true;
      }
      if (sample_to_remove <= min_) {
        min_stale_ = true;
      }
    } else {
      
      ++count_;
    }
    
    samples_[next_index_] = sample;
    sum_ += sample;
    sum_2_ += sample * sample;
    if (count_ == 1 || sample >= max_) {
      max_ = sample;
      max_stale_ = false;
    }
    if (count_ == 1 || sample <= min_) {
      min_ = sample;
      min_stale_ = false;
    }
    
    next_index_ = (next_index_ + 1) % max_count();
  }

  T ComputeSum() const {
    return static_cast<T>(sum_);
  }

  double ComputeMean() const {
    if (count_ == 0) {
      return 0.0;
    }
    return sum_ / count_;
  }

  T ComputeMax() const {
    if (max_stale_) {
      ASSERT(count_ > 0 &&
          "It shouldn't be possible for max_stale_ && count_ == 0");
      max_ = samples_[next_index_];
      for (size_t i = 1u; i < count_; i++) {
        max_ = _max(max_, samples_[(next_index_ + i) % max_count()]);
      }
      max_stale_ = false;
    }
    return max_;
  }

  T ComputeMin() const {
    if (min_stale_) {
      ASSERT(count_ > 0 &&
          "It shouldn't be possible for min_stale_ && count_ == 0");
      min_ = samples_[next_index_];
      for (size_t i = 1u; i < count_; i++) {
        min_ = _min(min_, samples_[(next_index_ + i) % max_count()]);
      }
      min_stale_ = false;
    }
    return min_;
  }

  
  
  
  double ComputeWeightedMean(double learning_rate) const {
    if (count_ < 1 || learning_rate <= 0.0 || learning_rate >= 1.0) {
      return ComputeMean();
    }
    double weighted_mean = 0.0;
    double current_weight = 1.0;
    double weight_sum = 0.0;
    const size_t max_size = max_count();
    for (size_t i = 0; i < count_; ++i) {
      current_weight *= learning_rate;
      weight_sum += current_weight;
      
      size_t index = (next_index_ + max_size - i - 1) % max_size;
      weighted_mean += current_weight * samples_[index];
    }
    return weighted_mean / weight_sum;
  }

  
  
  double ComputeVariance() const {
    if (count_ == 0) {
      return 0.0;
    }
    
    double count_inv = 1.0 / count_;
    double mean_2 = sum_2_ * count_inv;
    double mean = sum_ * count_inv;
    return mean_2 - (mean * mean);
  }

 private:
  size_t count_;
  size_t next_index_;
  double sum_;    
  double sum_2_;  
  mutable T max_;
  mutable bool max_stale_;
  mutable T min_;
  mutable bool min_stale_;
  std::vector<T> samples_;

  DISALLOW_COPY_AND_ASSIGN(RollingAccumulator);
};

}  

#endif  
