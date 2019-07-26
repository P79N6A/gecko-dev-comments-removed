









#include "webrtc/modules/audio_coding/neteq4/decision_logic_fax.h"

#include <assert.h>

#include <algorithm>

#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"

namespace webrtc {

Operations DecisionLogicFax::GetDecisionSpecialized(
    const SyncBuffer& sync_buffer,
    const Expand& expand,
    int decoder_frame_length,
    const RTPHeader* packet_header,
    Modes prev_mode,
    bool play_dtmf,
    bool* reset_decoder) {
  assert(playout_mode_ == kPlayoutFax || playout_mode_ == kPlayoutOff);
  uint32_t target_timestamp = sync_buffer.end_timestamp();
  uint32_t available_timestamp = 0;
  int is_cng_packet = 0;
  if (packet_header) {
    available_timestamp = packet_header->timestamp;
    is_cng_packet =
        decoder_database_->IsComfortNoise(packet_header->payloadType);
  }
  if (is_cng_packet) {
    if (static_cast<int32_t>((generated_noise_samples_ + target_timestamp)
        - available_timestamp) >= 0) {
      
      return kRfc3389Cng;
    } else {
      
      return kRfc3389CngNoPacket;
    }
  }
  if (!packet_header) {
    
    
    if (cng_state_ == kCngRfc3389On) {
      
      return kRfc3389CngNoPacket;
    } else if (cng_state_ == kCngInternalOn) {
      
      return kCodecInternalCng;
    } else {
      
      switch (playout_mode_) {
        case kPlayoutOff:
          return kAlternativePlc;
        case kPlayoutFax:
          return kAudioRepetition;
        default:
          assert(false);
          return kUndefined;
      }
    }
  } else if (target_timestamp == available_timestamp) {
    return kNormal;
  } else {
    if (static_cast<int32_t>((generated_noise_samples_ + target_timestamp)
        - available_timestamp) >= 0) {
      return kNormal;
    } else {
      
      
      
      if (cng_state_ == kCngRfc3389On) {
        return kRfc3389CngNoPacket;
      } else if (cng_state_ == kCngInternalOn) {
        return kCodecInternalCng;
      } else {
        
        
        switch (playout_mode_) {
          case kPlayoutOff:
            return kAlternativePlcIncreaseTimestamp;
          case kPlayoutFax:
            return kAudioRepetitionIncreaseTimestamp;
          default:
            assert(0);
            return kUndefined;
        }
      }
    }
  }
}


}  
