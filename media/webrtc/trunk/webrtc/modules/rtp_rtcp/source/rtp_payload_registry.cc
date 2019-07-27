









#include "webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h"

#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

RTPPayloadRegistry::RTPPayloadRegistry(
    RTPPayloadStrategy* rtp_payload_strategy)
    : crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      rtp_payload_strategy_(rtp_payload_strategy),
      red_payload_type_(-1),
      ulpfec_payload_type_(-1),
      incoming_payload_type_(-1),
      last_received_payload_type_(-1),
      last_received_media_payload_type_(-1),
      rtx_(false),
      payload_type_rtx_(-1),
      ssrc_rtx_(0) {}

RTPPayloadRegistry::~RTPPayloadRegistry() {
  while (!payload_type_map_.empty()) {
    RtpUtility::PayloadTypeMap::iterator it = payload_type_map_.begin();
    delete it->second;
    payload_type_map_.erase(it);
  }
}

int32_t RTPPayloadRegistry::RegisterReceivePayload(
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const int8_t payload_type,
    const uint32_t frequency,
    const uint8_t channels,
    const uint32_t rate,
    bool* created_new_payload) {
  assert(payload_type >= 0);
  assert(payload_name);
  *created_new_payload = false;

  
  switch (payload_type) {
    
    case 64:        
    case 72:        
    case 73:        
    case 74:        
    case 75:        
    case 76:        
    case 77:        
    case 78:        
    case 79:        
      LOG(LS_ERROR) << "Can't register invalid receiver payload type: "
                    << payload_type;
      return -1;
    default:
      break;
  }

  size_t payload_name_length = strlen(payload_name);

  CriticalSectionScoped cs(crit_sect_.get());

  RtpUtility::PayloadTypeMap::iterator it =
      payload_type_map_.find(payload_type);

  if (it != payload_type_map_.end()) {
    
    RtpUtility::Payload* payload = it->second;

    assert(payload);

    size_t name_length = strlen(payload->name);

    
    
    if (payload_name_length == name_length &&
        RtpUtility::StringCompare(
            payload->name, payload_name, payload_name_length)) {
      if (rtp_payload_strategy_->PayloadIsCompatible(*payload, frequency,
                                                     channels, rate)) {
        rtp_payload_strategy_->UpdatePayloadRate(payload, rate);
        return 0;
      }
    }
    LOG(LS_ERROR) << "Payload type already registered: " << payload_type;
    return -1;
  }

  if (rtp_payload_strategy_->CodecsMustBeUnique()) {
    DeregisterAudioCodecOrRedTypeRegardlessOfPayloadType(
        payload_name, payload_name_length, frequency, channels, rate);
  }

  RtpUtility::Payload* payload = NULL;

  
  if (RtpUtility::StringCompare(payload_name, "red", 3)) {
    red_payload_type_ = payload_type;
    payload = new RtpUtility::Payload;
    memset(payload, 0, sizeof(*payload));
    payload->audio = false;
    strncpy(payload->name, payload_name, RTP_PAYLOAD_NAME_SIZE - 1);
  } else if (RtpUtility::StringCompare(payload_name, "ulpfec", 3)) {
    ulpfec_payload_type_ = payload_type;
    payload = new RtpUtility::Payload;
    memset(payload, 0, sizeof(*payload));
    payload->audio = false;
    strncpy(payload->name, payload_name, RTP_PAYLOAD_NAME_SIZE - 1);
  } else {
    *created_new_payload = true;
    payload = rtp_payload_strategy_->CreatePayloadType(
        payload_name, payload_type, frequency, channels, rate);
  }
  payload_type_map_[payload_type] = payload;

  
  
  last_received_payload_type_ = -1;
  last_received_media_payload_type_ = -1;
  return 0;
}

int32_t RTPPayloadRegistry::DeRegisterReceivePayload(
    const int8_t payload_type) {
  CriticalSectionScoped cs(crit_sect_.get());
  RtpUtility::PayloadTypeMap::iterator it =
      payload_type_map_.find(payload_type);
  assert(it != payload_type_map_.end());
  delete it->second;
  payload_type_map_.erase(it);
  return 0;
}




void RTPPayloadRegistry::DeregisterAudioCodecOrRedTypeRegardlessOfPayloadType(
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const size_t payload_name_length,
    const uint32_t frequency,
    const uint8_t channels,
    const uint32_t rate) {
  RtpUtility::PayloadTypeMap::iterator iterator = payload_type_map_.begin();
  for (; iterator != payload_type_map_.end(); ++iterator) {
    RtpUtility::Payload* payload = iterator->second;
    size_t name_length = strlen(payload->name);

    if (payload_name_length == name_length &&
        RtpUtility::StringCompare(
            payload->name, payload_name, payload_name_length)) {
      
      
      if (payload->audio) {
        if (rtp_payload_strategy_->PayloadIsCompatible(*payload, frequency,
                                                       channels, rate)) {
          
          delete payload;
          payload_type_map_.erase(iterator);
          break;
        }
      } else if (RtpUtility::StringCompare(payload_name, "red", 3)) {
        delete payload;
        payload_type_map_.erase(iterator);
        break;
      }
    }
  }
}

int32_t RTPPayloadRegistry::ReceivePayloadType(
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const uint32_t frequency,
    const uint8_t channels,
    const uint32_t rate,
    int8_t* payload_type) const {
  assert(payload_type);
  size_t payload_name_length = strlen(payload_name);

  CriticalSectionScoped cs(crit_sect_.get());

  RtpUtility::PayloadTypeMap::const_iterator it = payload_type_map_.begin();

  for (; it != payload_type_map_.end(); ++it) {
    RtpUtility::Payload* payload = it->second;
    assert(payload);

    size_t name_length = strlen(payload->name);
    if (payload_name_length == name_length &&
        RtpUtility::StringCompare(
            payload->name, payload_name, payload_name_length)) {
      
      if (payload->audio) {
        if (rate == 0) {
          
          if (payload->typeSpecific.Audio.frequency == frequency &&
              payload->typeSpecific.Audio.channels == channels) {
            *payload_type = it->first;
            return 0;
          }
        } else {
          
          if (payload->typeSpecific.Audio.frequency == frequency &&
              payload->typeSpecific.Audio.channels == channels &&
              payload->typeSpecific.Audio.rate == rate) {
            
            *payload_type = it->first;
            return 0;
          }
        }
      } else {
        
        *payload_type = it->first;
        return 0;
      }
    }
  }
  return -1;
}

bool RTPPayloadRegistry::RtxEnabled() const {
  CriticalSectionScoped cs(crit_sect_.get());
  return rtx_;
}

bool RTPPayloadRegistry::IsRtx(const RTPHeader& header) const {
  CriticalSectionScoped cs(crit_sect_.get());
  return IsRtxInternal(header);
}

bool RTPPayloadRegistry::IsRtxInternal(const RTPHeader& header) const {
  return rtx_ && ssrc_rtx_ == header.ssrc;
}

bool RTPPayloadRegistry::RestoreOriginalPacket(uint8_t** restored_packet,
                                               const uint8_t* packet,
                                               int* packet_length,
                                               uint32_t original_ssrc,
                                               const RTPHeader& header) const {
  if (kRtxHeaderSize + header.headerLength > *packet_length) {
    return false;
  }
  const uint8_t* rtx_header = packet + header.headerLength;
  uint16_t original_sequence_number = (rtx_header[0] << 8) + rtx_header[1];

  
  memcpy(*restored_packet, packet, header.headerLength);
  memcpy(*restored_packet + header.headerLength,
         packet + header.headerLength + kRtxHeaderSize,
         *packet_length - header.headerLength - kRtxHeaderSize);
  *packet_length -= kRtxHeaderSize;

  
  RtpUtility::AssignUWord16ToBuffer(*restored_packet + 2,
                                    original_sequence_number);
  RtpUtility::AssignUWord32ToBuffer(*restored_packet + 8, original_ssrc);

  CriticalSectionScoped cs(crit_sect_.get());

  if (payload_type_rtx_ != -1) {
    if (header.payloadType == payload_type_rtx_ &&
        incoming_payload_type_ != -1) {
      (*restored_packet)[1] = static_cast<uint8_t>(incoming_payload_type_);
      if (header.markerBit) {
        (*restored_packet)[1] |= kRtpMarkerBitMask;  
      }
    } else {
      LOG(LS_WARNING) << "Incorrect RTX configuration, dropping packet.";
      return false;
    }
  }
  return true;
}

void RTPPayloadRegistry::SetRtxSsrc(uint32_t ssrc) {
  CriticalSectionScoped cs(crit_sect_.get());
  ssrc_rtx_ = ssrc;
  rtx_ = true;
}

void RTPPayloadRegistry::SetRtxPayloadType(int payload_type) {
  CriticalSectionScoped cs(crit_sect_.get());
  assert(payload_type >= 0);
  payload_type_rtx_ = payload_type;
  rtx_ = true;
}

bool RTPPayloadRegistry::IsRed(const RTPHeader& header) const {
  CriticalSectionScoped cs(crit_sect_.get());
  return red_payload_type_ == header.payloadType;
}

bool RTPPayloadRegistry::IsEncapsulated(const RTPHeader& header) const {
  return IsRed(header) || IsRtx(header);
}

bool RTPPayloadRegistry::GetPayloadSpecifics(uint8_t payload_type,
                                             PayloadUnion* payload) const {
  CriticalSectionScoped cs(crit_sect_.get());
  RtpUtility::PayloadTypeMap::const_iterator it =
      payload_type_map_.find(payload_type);

  
  if (it == payload_type_map_.end()) {
    return false;
  }
  *payload = it->second->typeSpecific;
  return true;
}

int RTPPayloadRegistry::GetPayloadTypeFrequency(
    uint8_t payload_type) const {
  RtpUtility::Payload* payload;
  if (!PayloadTypeToPayload(payload_type, payload)) {
    return -1;
  }
  CriticalSectionScoped cs(crit_sect_.get());
  return rtp_payload_strategy_->GetPayloadTypeFrequency(*payload);
}

bool RTPPayloadRegistry::PayloadTypeToPayload(
    const uint8_t payload_type,
    RtpUtility::Payload*& payload) const {
  CriticalSectionScoped cs(crit_sect_.get());

  RtpUtility::PayloadTypeMap::const_iterator it =
      payload_type_map_.find(payload_type);

  
  if (it == payload_type_map_.end()) {
    return false;
  }

  payload = it->second;
  return true;
}

void RTPPayloadRegistry::SetIncomingPayloadType(const RTPHeader& header) {
  CriticalSectionScoped cs(crit_sect_.get());
  if (!IsRtxInternal(header))
    incoming_payload_type_ = header.payloadType;
}

bool RTPPayloadRegistry::ReportMediaPayloadType(uint8_t media_payload_type) {
  CriticalSectionScoped cs(crit_sect_.get());
  if (last_received_media_payload_type_ == media_payload_type) {
    
    return true;
  }
  last_received_media_payload_type_ = media_payload_type;
  return false;
}

class RTPPayloadAudioStrategy : public RTPPayloadStrategy {
 public:
  virtual bool CodecsMustBeUnique() const OVERRIDE { return true; }

  virtual bool PayloadIsCompatible(const RtpUtility::Payload& payload,
                                   const uint32_t frequency,
                                   const uint8_t channels,
                                   const uint32_t rate) const OVERRIDE {
    return
        payload.audio &&
        payload.typeSpecific.Audio.frequency == frequency &&
        payload.typeSpecific.Audio.channels == channels &&
        (payload.typeSpecific.Audio.rate == rate ||
            payload.typeSpecific.Audio.rate == 0 || rate == 0);
  }

  virtual void UpdatePayloadRate(RtpUtility::Payload* payload,
                                 const uint32_t rate) const OVERRIDE {
    payload->typeSpecific.Audio.rate = rate;
  }

  virtual RtpUtility::Payload* CreatePayloadType(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payloadType,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) const OVERRIDE {
    RtpUtility::Payload* payload = new RtpUtility::Payload;
    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
    assert(frequency >= 1000);
    payload->typeSpecific.Audio.frequency = frequency;
    payload->typeSpecific.Audio.channels = channels;
    payload->typeSpecific.Audio.rate = rate;
    payload->audio = true;
    return payload;
  }

  int GetPayloadTypeFrequency(const RtpUtility::Payload& payload) const {
    return payload.typeSpecific.Audio.frequency;
  }
};

class RTPPayloadVideoStrategy : public RTPPayloadStrategy {
 public:
  virtual bool CodecsMustBeUnique() const OVERRIDE { return false; }

  virtual bool PayloadIsCompatible(const RtpUtility::Payload& payload,
                                   const uint32_t frequency,
                                   const uint8_t channels,
                                   const uint32_t rate) const OVERRIDE {
    return !payload.audio;
  }

  virtual void UpdatePayloadRate(RtpUtility::Payload* payload,
                                 const uint32_t rate) const OVERRIDE {
    payload->typeSpecific.Video.maxRate = rate;
  }

  virtual RtpUtility::Payload* CreatePayloadType(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payloadType,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) const OVERRIDE {
    RtpVideoCodecTypes videoType = kRtpVideoGeneric;
    if (RtpUtility::StringCompare(payloadName, "VP8", 3)) {
      videoType = kRtpVideoVp8;
    } else if (RtpUtility::StringCompare(payloadName, "VP9", 3)) {
      videoType = kRtpVideoVp9;
    } else if (RtpUtility::StringCompare(payloadName, "H264", 4)) {
      videoType = kRtpVideoH264;
    } else if (RtpUtility::StringCompare(payloadName, "I420", 4)) {
      videoType = kRtpVideoGeneric;
    } else if (RtpUtility::StringCompare(payloadName, "ULPFEC", 6)) {
      videoType = kRtpVideoNone;
    } else {
      videoType = kRtpVideoGeneric;
    }
    RtpUtility::Payload* payload = new RtpUtility::Payload;

    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
    payload->typeSpecific.Video.videoCodecType = videoType;
    payload->typeSpecific.Video.maxRate = rate;
    payload->audio = false;
    return payload;
  }

  int GetPayloadTypeFrequency(const RtpUtility::Payload& payload) const {
    return kVideoPayloadTypeFrequency;
  }
};

RTPPayloadStrategy* RTPPayloadStrategy::CreateStrategy(
    const bool handling_audio) {
  if (handling_audio) {
    return new RTPPayloadAudioStrategy();
  } else {
    return new RTPPayloadVideoStrategy();
  }
}

}  
