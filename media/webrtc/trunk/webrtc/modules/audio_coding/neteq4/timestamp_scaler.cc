









#include "webrtc/modules/audio_coding/neteq4/timestamp_scaler.h"

#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

void TimestampScaler::ToInternal(Packet* packet) {
  if (!packet) {
    return;
  }
  packet->header.timestamp = ToInternal(packet->header.timestamp,
                                        packet->header.payloadType);
}

void TimestampScaler::ToInternal(PacketList* packet_list) {
  PacketList::iterator it;
  for (it = packet_list->begin(); it != packet_list->end(); ++it) {
    ToInternal(*it);
  }
}

uint32_t TimestampScaler::ToInternal(uint32_t external_timestamp,
                                     uint8_t rtp_payload_type) {
  const DecoderDatabase::DecoderInfo* info =
      decoder_database_.GetDecoderInfo(rtp_payload_type);
  if (!info) {
    
    return external_timestamp;
  }
  switch (info->codec_type) {
    case kDecoderG722:
    case kDecoderG722_2ch: {
      
      
      numerator_ = 2;
      denominator_ = 1;
      break;
    }
    case kDecoderOpus:
    case kDecoderOpus_2ch:
    case kDecoderISACfb:
    case kDecoderCNGswb48kHz: {
      
      
      
      
      numerator_ = 2;
      denominator_ = 3;
    }
    case kDecoderAVT:
    case kDecoderCNGnb:
    case kDecoderCNGwb:
    case kDecoderCNGswb32kHz: {
      
      break;
    }
    default: {
      
      numerator_ = 1;
      denominator_ = 1;
      break;
    }
  }

  if (!(numerator_ == 1 && denominator_ == 1)) {
    
    if (!first_packet_received_) {
      external_ref_ = external_timestamp;
      internal_ref_ = external_timestamp;
      first_packet_received_ = true;
    }
    int32_t external_diff = external_timestamp - external_ref_;
    assert(denominator_ > 0);  
    external_ref_ = external_timestamp;
    internal_ref_ += (external_diff * numerator_) / denominator_;
    LOG(LS_VERBOSE) << "Converting timestamp: " << external_timestamp <<
        " -> " << internal_ref_;
    return internal_ref_;
  } else {
    
    return external_timestamp;
  }
}


uint32_t TimestampScaler::ToExternal(uint32_t internal_timestamp) const {
  if (!first_packet_received_ || (numerator_ == 1 && denominator_ == 1)) {
    
    return internal_timestamp;
  } else {
    int32_t internal_diff = internal_timestamp - internal_ref_;
    assert(numerator_ > 0);  
    
    
    return external_ref_ + (internal_diff * denominator_) / numerator_;
  }
}

}  
