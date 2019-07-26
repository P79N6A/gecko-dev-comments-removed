









#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"

#include <assert.h>
#include <math.h>

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"
#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

DelayManager::DelayManager(int max_packets_in_buffer,
                           DelayPeakDetector* peak_detector)
    : first_packet_received_(false),
      max_packets_in_buffer_(max_packets_in_buffer),
      iat_vector_(kMaxIat + 1, 0),
      iat_factor_(0),
      packet_iat_count_ms_(0),
      base_target_level_(4),  
      target_level_(base_target_level_ << 8),  
      packet_len_ms_(0),
      streaming_mode_(false),
      last_seq_no_(0),
      last_timestamp_(0),
      extra_delay_ms_(0),
      iat_cumulative_sum_(0),
      max_iat_cumulative_sum_(0),
      max_timer_ms_(0),
      peak_detector_(*peak_detector),
      last_pack_cng_or_dtmf_(1) {
  assert(peak_detector);  
  Reset();
}




void DelayManager::ResetHistogram() {
  
  
  uint16_t temp_prob = 0x4002;  
  IATVector::iterator it = iat_vector_.begin();
  for (; it < iat_vector_.end(); it++) {
    temp_prob >>= 1;
    (*it) = temp_prob << 16;
  }
  base_target_level_ = 4;
  target_level_ = base_target_level_ << 8;
}

int DelayManager::Update(uint16_t sequence_number,
                         uint32_t timestamp,
                         int sample_rate_hz) {
  if (sample_rate_hz <= 0) {
    return -1;
  }

  if (!first_packet_received_) {
    
    packet_iat_count_ms_ = 0;
    last_seq_no_ = sequence_number;
    last_timestamp_ = timestamp;
    first_packet_received_ = true;
    return 0;
  }

  
  
  
  int packet_len_ms;
  if ((timestamp <= last_timestamp_) || (sequence_number <= last_seq_no_)) {
    
    packet_len_ms = packet_len_ms_;
  } else {
    
    int packet_len_samp =
        static_cast<uint32_t>(timestamp - last_timestamp_) /
        static_cast<uint16_t>(sequence_number - last_seq_no_);
    packet_len_ms = (1000 * packet_len_samp) / sample_rate_hz;
  }

  if (packet_len_ms > 0) {
    
    
    
    
    int iat_packets = packet_iat_count_ms_ / packet_len_ms;

    if (streaming_mode_) {
      UpdateCumulativeSums(packet_len_ms, sequence_number);
    }

    
    if (sequence_number > last_seq_no_ + 1) {
      
      
      
      
      
      iat_packets -= sequence_number - last_seq_no_ - 1;
      iat_packets = std::max(iat_packets, 0);
    } else if (sequence_number < last_seq_no_) {
      
      
      iat_packets += last_seq_no_ + 1 - sequence_number;
    }

    
    const int max_iat = kMaxIat;
    iat_packets = std::min(iat_packets, max_iat);
    UpdateHistogram(iat_packets);
    
    target_level_ = CalculateTargetLevel(iat_packets);
    if (streaming_mode_) {
      target_level_ = std::max(target_level_, max_iat_cumulative_sum_);
    }

    LimitTargetLevel();
  }  

  
  packet_iat_count_ms_ = 0;
  last_seq_no_ = sequence_number;
  last_timestamp_ = timestamp;
  return 0;
}

void DelayManager::UpdateCumulativeSums(int packet_len_ms,
                                        uint16_t sequence_number) {
  
  
  int iat_packets_q8 = (packet_iat_count_ms_ << 8) / packet_len_ms;
  
  
  iat_cumulative_sum_ += (iat_packets_q8 -
      (static_cast<int>(sequence_number - last_seq_no_) << 8));
  
  iat_cumulative_sum_ -= kCumulativeSumDrift;
  
  iat_cumulative_sum_ = std::max(iat_cumulative_sum_, 0);
  if (iat_cumulative_sum_ > max_iat_cumulative_sum_) {
    
    max_iat_cumulative_sum_ = iat_cumulative_sum_;
    max_timer_ms_ = 0;
  }
  if (max_timer_ms_ > kMaxStreamingPeakPeriodMs) {
    
    max_iat_cumulative_sum_ -= kCumulativeSumDrift;
  }
}














void DelayManager::UpdateHistogram(size_t iat_packets) {
  assert(iat_packets < iat_vector_.size());
  int vector_sum = 0;  
  
  for (IATVector::iterator it = iat_vector_.begin();
      it != iat_vector_.end(); ++it) {
    *it = (static_cast<int64_t>(*it) * iat_factor_) >> 15;
    vector_sum += *it;
  }

  
  
  
  iat_vector_[iat_packets] += (32768 - iat_factor_) << 15;
  vector_sum += (32768 - iat_factor_) << 15;  

  
  
  vector_sum -= 1 << 30;  
  if (vector_sum != 0) {
    
    int flip_sign = vector_sum > 0 ? -1 : 1;
    IATVector::iterator it = iat_vector_.begin();
    while (it != iat_vector_.end() && abs(vector_sum) > 0) {
      
      int correction = flip_sign * std::min(abs(vector_sum), (*it) >> 4);
      *it += correction;
      vector_sum += correction;
      ++it;
    }
  }
  assert(vector_sum == 0);  

  
  
  iat_factor_ += (kIatFactor_ - iat_factor_ + 3) >> 2;
}








void DelayManager::LimitTargetLevel() {
  int max_buffer_len = max_packets_in_buffer_;
  if (extra_delay_ms_ > 0 && packet_len_ms_ > 0) {
    max_buffer_len -= extra_delay_ms_ / packet_len_ms_;
    max_buffer_len = std::max(max_buffer_len, 1);  
  }
  max_buffer_len = (3 * (max_buffer_len << 8)) / 4;  
  target_level_ = std::min(target_level_, max_buffer_len);
}

int DelayManager::CalculateTargetLevel(int iat_packets) {
  int limit_probability = kLimitProbability;
  if (streaming_mode_) {
    limit_probability = kLimitProbabilityStreaming;
  }

  
  
  
  
  
  
  
  
  
  size_t index = 0;  
  int sum = 1 << 30;  
  sum -= iat_vector_[index];  

  do {
    
    
    ++index;
    sum -= iat_vector_[index];
  } while ((sum > limit_probability) && (index < iat_vector_.size() - 1));

  
  int target_level = index;
  base_target_level_ = index;

  
  bool delay_peak_found = peak_detector_.Update(iat_packets, target_level);
  if (delay_peak_found) {
    target_level = std::max(static_cast<int>(target_level),
                            peak_detector_.MaxPeakHeight());
  }

  
  target_level = std::max(target_level, 1);
  
  target_level_ = target_level << 8;
  return target_level_;
}

int DelayManager::SetPacketAudioLength(int length_ms) {
  if (length_ms <= 0) {
    LOG_F(LS_ERROR) << "length_ms = " << length_ms;
    return -1;
  }
  packet_len_ms_ = length_ms;
  peak_detector_.SetPacketAudioLength(packet_len_ms_);
  packet_iat_count_ms_ = 0;
  last_pack_cng_or_dtmf_ = 1;  
  return 0;
}


void DelayManager::Reset() {
  packet_len_ms_ = 0;  
  streaming_mode_ = false;
  peak_detector_.Reset();
  ResetHistogram();  
  iat_factor_ = 0;  
  packet_iat_count_ms_ = 0;
  max_timer_ms_ = 0;
  iat_cumulative_sum_ = 0;
  max_iat_cumulative_sum_ = 0;
  last_pack_cng_or_dtmf_ = 1;
}

int DelayManager::AverageIAT() const {
  int32_t sum_q24 = 0;
  assert(iat_vector_.size() == 65);  
  for (size_t i = 0; i < iat_vector_.size(); ++i) {
    
    sum_q24 += (iat_vector_[i] >> 6) * i;
  }
  
  sum_q24 -= (1 << 24);
  
  
  return ((sum_q24 >> 7) * 15625) >> 11;
}

bool DelayManager::PeakFound() const {
  return peak_detector_.peak_found();
}

void DelayManager::UpdateCounters(int elapsed_time_ms) {
  packet_iat_count_ms_ += elapsed_time_ms;
  peak_detector_.IncrementCounter(elapsed_time_ms);
  max_timer_ms_ += elapsed_time_ms;
}

void DelayManager::BufferLimits(int* lower_limit, int* higher_limit) const {
  if (!lower_limit || !higher_limit) {
    LOG_F(LS_ERROR) << "NULL pointers supplied as input";
    assert(false);
    return;
  }

  int extra_delay_packets_q8 = 0;
  int window_20ms = 0x7FFF;  
  if (packet_len_ms_ > 0) {
    extra_delay_packets_q8 = (extra_delay_ms_ << 8) / packet_len_ms_;
    window_20ms = (20 << 8) / packet_len_ms_;
  }
  
  
  *lower_limit = (target_level_ * 3) / 4 + extra_delay_packets_q8;
  
  
  *higher_limit = std::max(target_level_ + extra_delay_packets_q8,
                           *lower_limit + window_20ms);
}

int DelayManager::TargetLevel() const {
  if (packet_len_ms_ > 0) {
    
    return target_level_ + (extra_delay_ms_ << 8) / packet_len_ms_;
  } else {
    
    return target_level_;
  }
}

void DelayManager::LastDecoderType(NetEqDecoder decoder_type) {
  if (decoder_type == kDecoderAVT ||
      decoder_type == kDecoderCNGnb ||
      decoder_type == kDecoderCNGwb ||
      decoder_type == kDecoderCNGswb32kHz ||
      decoder_type == kDecoderCNGswb48kHz) {
    last_pack_cng_or_dtmf_ = 1;
  } else if (last_pack_cng_or_dtmf_ != 0) {
    last_pack_cng_or_dtmf_ = -1;
  }
}
}  
