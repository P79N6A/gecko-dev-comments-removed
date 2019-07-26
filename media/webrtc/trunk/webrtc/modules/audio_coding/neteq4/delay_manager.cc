









#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"

#include <assert.h>
#include <math.h>

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"
#include "webrtc/modules/interface/module_common_types.h"
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
      minimum_delay_ms_(0),
      least_required_delay_ms_(target_level_),
      maximum_delay_ms_(target_level_),
      iat_cumulative_sum_(0),
      max_iat_cumulative_sum_(0),
      max_timer_ms_(0),
      peak_detector_(*peak_detector),
      last_pack_cng_or_dtmf_(1) {
  assert(peak_detector);  
  Reset();
}

DelayManager::~DelayManager() {}

const DelayManager::IATVector& DelayManager::iat_vector() const {
  return iat_vector_;
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
  if (!IsNewerTimestamp(timestamp, last_timestamp_) ||
      !IsNewerSequenceNumber(sequence_number, last_seq_no_)) {
    
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

    
    if (IsNewerSequenceNumber(sequence_number, last_seq_no_ + 1)) {
      
      
      
      iat_packets -= static_cast<uint16_t>(sequence_number - last_seq_no_ - 1);
      iat_packets = std::max(iat_packets, 0);
    } else if (!IsNewerSequenceNumber(sequence_number, last_seq_no_)) {
      iat_packets += static_cast<uint16_t>(last_seq_no_ + 1 - sequence_number);
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
  least_required_delay_ms_ = (target_level_ * packet_len_ms_) >> 8;

  if (packet_len_ms_ > 0 && minimum_delay_ms_ > 0) {
    int minimum_delay_packet_q8 =  (minimum_delay_ms_ << 8) / packet_len_ms_;
    target_level_ = std::max(target_level_, minimum_delay_packet_q8);
  }

  if (maximum_delay_ms_ > 0 && packet_len_ms_ > 0) {
    int maximum_delay_packet_q8 = (maximum_delay_ms_ << 8) / packet_len_ms_;
    target_level_ = std::min(target_level_, maximum_delay_packet_q8);
  }

  
  int max_buffer_packets_q8 = (3 * (max_packets_in_buffer_ << 8)) / 4;
  target_level_ = std::min(target_level_, max_buffer_packets_q8);

  
  target_level_ = std::max(target_level_, 1 << 8);
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

  
  int target_level = static_cast<int>(index);
  base_target_level_ = static_cast<int>(index);

  
  bool delay_peak_found = peak_detector_.Update(iat_packets, target_level);
  if (delay_peak_found) {
    target_level = std::max(target_level, peak_detector_.MaxPeakHeight());
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
  
  
  
  const int iat_vec_size = static_cast<int>(iat_vector_.size());
  assert(iat_vector_.size() == 65);  
  for (int i = 0; i < iat_vec_size; ++i) {
    
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

void DelayManager::ResetPacketIatCount() { packet_iat_count_ms_ = 0; }




void DelayManager::BufferLimits(int* lower_limit, int* higher_limit) const {
  if (!lower_limit || !higher_limit) {
    LOG_F(LS_ERROR) << "NULL pointers supplied as input";
    assert(false);
    return;
  }

  int window_20ms = 0x7FFF;  
  if (packet_len_ms_ > 0) {
    window_20ms = (20 << 8) / packet_len_ms_;
  }

  
  *lower_limit = (target_level_ * 3) / 4;
  
  
  *higher_limit = std::max(target_level_, *lower_limit + window_20ms);
}

int DelayManager::TargetLevel() const {
  return target_level_;
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

bool DelayManager::SetMinimumDelay(int delay_ms) {
  
  
  
  if ((maximum_delay_ms_ > 0 && delay_ms > maximum_delay_ms_) ||
      (packet_len_ms_ > 0 &&
          delay_ms > 3 * max_packets_in_buffer_ * packet_len_ms_ / 4)) {
    return false;
  }
  minimum_delay_ms_ = delay_ms;
  return true;
}

bool DelayManager::SetMaximumDelay(int delay_ms) {
  if (delay_ms == 0) {
    
    maximum_delay_ms_ = 0;
    return true;
  } else if (delay_ms < minimum_delay_ms_ || delay_ms < packet_len_ms_) {
    
    return false;
  }
  maximum_delay_ms_ = delay_ms;
  return true;
}

int DelayManager::least_required_delay_ms() const {
  return least_required_delay_ms_;
}

int DelayManager::base_target_level() const { return base_target_level_; }
void DelayManager::set_streaming_mode(bool value) { streaming_mode_ = value; }
int DelayManager::last_pack_cng_or_dtmf() const {
  return last_pack_cng_or_dtmf_;
}

void DelayManager::set_last_pack_cng_or_dtmf(int value) {
  last_pack_cng_or_dtmf_ = value;
}
}  
