









#include "webrtc/modules/audio_coding/neteq4/decision_logic_normal.h"

#include <assert.h>

#include <algorithm>

#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"
#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"
#include "webrtc/modules/audio_coding/neteq4/expand.h"
#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"
#include "webrtc/modules/interface/module_common_types.h"

namespace webrtc {

Operations DecisionLogicNormal::GetDecisionSpecialized(
    const SyncBuffer& sync_buffer,
    const Expand& expand,
    int decoder_frame_length,
    const RTPHeader* packet_header,
    Modes prev_mode,
    bool play_dtmf,
    bool* reset_decoder) {
  assert(playout_mode_ == kPlayoutOn || playout_mode_ == kPlayoutStreaming);
  
  if (prev_mode == kModeError) {
    if (!packet_header) {
      return kExpand;
    } else {
      return kUndefined;  
    }
  }

  uint32_t target_timestamp = sync_buffer.end_timestamp();
  uint32_t available_timestamp = 0;
  bool is_cng_packet = false;
  if (packet_header) {
    available_timestamp = packet_header->timestamp;
    is_cng_packet =
        decoder_database_->IsComfortNoise(packet_header->payloadType);
  }

  if (is_cng_packet) {
    return CngOperation(prev_mode, target_timestamp, available_timestamp);
  }

  
  if (!packet_header) {
    return NoPacket(play_dtmf);
  }

  
  
  if (num_consecutive_expands_ > kReinitAfterExpands) {
    *reset_decoder = true;
    return kNormal;
  }

  
  if (target_timestamp == available_timestamp) {
    return ExpectedPacketAvailable(prev_mode, play_dtmf);
  } else if (IsNewerTimestamp(available_timestamp, target_timestamp)) {
    return FuturePacketAvailable(sync_buffer, expand, decoder_frame_length,
                                 prev_mode, target_timestamp,
                                 available_timestamp, play_dtmf);
  } else {
    
    
    return kUndefined;
  }
}

Operations DecisionLogicNormal::CngOperation(Modes prev_mode,
                                             uint32_t target_timestamp,
                                             uint32_t available_timestamp) {
  
  int32_t timestamp_diff = (generated_noise_samples_ + target_timestamp) -
      available_timestamp;
  int32_t optimal_level_samp =
      (delay_manager_->TargetLevel() * packet_length_samples_) >> 8;
  int32_t excess_waiting_time_samp = -timestamp_diff - optimal_level_samp;

  if (excess_waiting_time_samp > optimal_level_samp / 2) {
    
    
    
    generated_noise_samples_ += excess_waiting_time_samp;
    timestamp_diff += excess_waiting_time_samp;
  }

  if (timestamp_diff < 0 && prev_mode == kModeRfc3389Cng) {
    
    
    return kRfc3389CngNoPacket;
  } else {
    
    return kRfc3389Cng;
  }
}

Operations DecisionLogicNormal::NoPacket(bool play_dtmf) {
  if (cng_state_ == kCngRfc3389On) {
    
    return kRfc3389CngNoPacket;
  } else if (cng_state_ == kCngInternalOn) {
    
    return kCodecInternalCng;
  } else if (play_dtmf) {
    return kDtmf;
  } else {
    
    return kExpand;
  }
}

Operations DecisionLogicNormal::ExpectedPacketAvailable(Modes prev_mode,
                                                        bool play_dtmf) {
  if (prev_mode != kModeExpand && !play_dtmf) {
    
    int low_limit, high_limit;
    delay_manager_->BufferLimits(&low_limit, &high_limit);
    if ((buffer_level_filter_->filtered_current_level() >= high_limit &&
        TimescaleAllowed()) ||
        buffer_level_filter_->filtered_current_level() >= high_limit << 2) {
      
      
      return kAccelerate;
    } else if ((buffer_level_filter_->filtered_current_level() < low_limit)
        && TimescaleAllowed()) {
      return kPreemptiveExpand;
    }
  }
  return kNormal;
}

Operations DecisionLogicNormal::FuturePacketAvailable(
    const SyncBuffer& sync_buffer,
    const Expand& expand,
    int decoder_frame_length,
    Modes prev_mode,
    uint32_t target_timestamp,
    uint32_t available_timestamp,
    bool play_dtmf) {
  
  
  
  uint32_t timestamp_leap = available_timestamp - target_timestamp;
  if ((prev_mode == kModeExpand) &&
      !ReinitAfterExpands(timestamp_leap) &&
      !MaxWaitForPacket() &&
      PacketTooEarly(timestamp_leap) &&
      UnderTargetLevel()) {
    if (play_dtmf) {
      
      return kDtmf;
    } else {
      
      return kExpand;
    }
  }

  const int samples_left = static_cast<int>(sync_buffer.FutureLength() -
      expand.overlap_length());
  const int cur_size_samples = samples_left +
      packet_buffer_.NumPacketsInBuffer() * decoder_frame_length;

  
  if (prev_mode == kModeRfc3389Cng ||
      prev_mode == kModeCodecInternalCng) {
    
    
    
    
    int32_t timestamp_diff = (generated_noise_samples_ + target_timestamp) -
        available_timestamp;
    if (timestamp_diff >= 0 ||
        cur_size_samples >
        4 * ((delay_manager_->TargetLevel() * packet_length_samples_) >> 8)) {
      
      return kNormal;
    } else {
      
      if (prev_mode == kModeRfc3389Cng) {
        return kRfc3389CngNoPacket;
      } else {  
        return kCodecInternalCng;
      }
    }
  }
  
  
  
  if (prev_mode == kModeExpand ||
      (decoder_frame_length < output_size_samples_ &&
       cur_size_samples > kAllowMergeWithoutExpandMs * fs_mult_ * 8)) {
    return kMerge;
  } else if (play_dtmf) {
    
    return kDtmf;
  } else {
    return kExpand;
  }
}

bool DecisionLogicNormal::UnderTargetLevel() const {
  return buffer_level_filter_->filtered_current_level() <=
      delay_manager_->TargetLevel();
}

bool DecisionLogicNormal::ReinitAfterExpands(uint32_t timestamp_leap) const {
  return timestamp_leap >=
      static_cast<uint32_t>(output_size_samples_ * kReinitAfterExpands);
}

bool DecisionLogicNormal::PacketTooEarly(uint32_t timestamp_leap) const {
  return timestamp_leap >
      static_cast<uint32_t>(output_size_samples_ * num_consecutive_expands_);
}

bool DecisionLogicNormal::MaxWaitForPacket() const {
  return num_consecutive_expands_ >= kMaxWaitForPacket;
}

}  
