









#include "webrtc/modules/rtp_rtcp/source/rtp_payload_registry.h"

#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

RTPPayloadRegistry::RTPPayloadRegistry(
    const int32_t id,
    RTPPayloadStrategy* rtp_payload_strategy)
    : id_(id),
      rtp_payload_strategy_(rtp_payload_strategy),
      red_payload_type_(-1),
      last_received_payload_type_(-1),
      last_received_media_payload_type_(-1) {
}

RTPPayloadRegistry::~RTPPayloadRegistry() {
  while (!payload_type_map_.empty()) {
    ModuleRTPUtility::PayloadTypeMap::iterator it = payload_type_map_.begin();
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
      WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, id_,
                   "%s invalid payloadtype:%d",
                   __FUNCTION__, payload_type);
      return -1;
    default:
      break;
  }

  size_t payload_name_length = strlen(payload_name);

  ModuleRTPUtility::PayloadTypeMap::iterator it =
    payload_type_map_.find(payload_type);

  if (it != payload_type_map_.end()) {
    
    ModuleRTPUtility::Payload* payload = it->second;

    assert(payload);

    size_t name_length = strlen(payload->name);

    
    
    if (payload_name_length == name_length &&
        ModuleRTPUtility::StringCompare(
            payload->name, payload_name, payload_name_length)) {
      if (rtp_payload_strategy_->PayloadIsCompatible(*payload, frequency,
                                                     channels, rate)) {
        rtp_payload_strategy_->UpdatePayloadRate(payload, rate);
        return 0;
      }
    }
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, id_,
                 "%s invalid argument payload_type:%d already registered",
                 __FUNCTION__, payload_type);
    return -1;
  }

  if (rtp_payload_strategy_->CodecsMustBeUnique()) {
    DeregisterAudioCodecOrRedTypeRegardlessOfPayloadType(
        payload_name, payload_name_length, frequency, channels, rate);
  }

  ModuleRTPUtility::Payload* payload = NULL;

  
  if (ModuleRTPUtility::StringCompare(payload_name, "red", 3)) {
    red_payload_type_ = payload_type;
    payload = new ModuleRTPUtility::Payload;
    payload->audio = false;
    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
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
  ModuleRTPUtility::PayloadTypeMap::iterator it =
    payload_type_map_.find(payload_type);

  if (it == payload_type_map_.end()) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, id_,
                 "%s failed to find payload_type:%d",
                 __FUNCTION__, payload_type);
    return -1;
  }
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
  ModuleRTPUtility::PayloadTypeMap::iterator iterator =
      payload_type_map_.begin();
  for (; iterator != payload_type_map_.end(); ++iterator) {
    ModuleRTPUtility::Payload* payload = iterator->second;
    size_t name_length = strlen(payload->name);

    if (payload_name_length == name_length
        && ModuleRTPUtility::StringCompare(payload->name, payload_name,
                                           payload_name_length)) {
      
      
      if (payload->audio) {
        if (rtp_payload_strategy_->PayloadIsCompatible(*payload, frequency,
                                                       channels, rate)) {
          
          delete payload;
          payload_type_map_.erase(iterator);
          break;
        }
      } else if (ModuleRTPUtility::StringCompare(payload_name, "red", 3)) {
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
  if (payload_type == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, id_,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  size_t payload_name_length = strlen(payload_name);

  ModuleRTPUtility::PayloadTypeMap::const_iterator it =
      payload_type_map_.begin();

  for (; it != payload_type_map_.end(); ++it) {
    ModuleRTPUtility::Payload* payload = it->second;
    assert(payload);

    size_t name_length = strlen(payload->name);
    if (payload_name_length == name_length &&
        ModuleRTPUtility::StringCompare(
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

int32_t RTPPayloadRegistry::PayloadTypeToPayload(
  const uint8_t payload_type,
  ModuleRTPUtility::Payload*& payload) const {

  ModuleRTPUtility::PayloadTypeMap::const_iterator it =
    payload_type_map_.find(payload_type);

  
  if (it == payload_type_map_.end()) {
    return -1;
  }
  payload = it->second;
  return 0;
}

bool RTPPayloadRegistry::ReportMediaPayloadType(
    uint8_t media_payload_type) {
  if (last_received_media_payload_type_ == media_payload_type) {
    
    return true;
  }
  last_received_media_payload_type_ = media_payload_type;
  return false;
}

class RTPPayloadAudioStrategy : public RTPPayloadStrategy {
 public:
  bool CodecsMustBeUnique() const { return true; }

  bool PayloadIsCompatible(
       const ModuleRTPUtility::Payload& payload,
       const uint32_t frequency,
       const uint8_t channels,
       const uint32_t rate) const {
    return
        payload.audio &&
        payload.typeSpecific.Audio.frequency == frequency &&
        payload.typeSpecific.Audio.channels == channels &&
        (payload.typeSpecific.Audio.rate == rate ||
            payload.typeSpecific.Audio.rate == 0 || rate == 0);
  }

  void UpdatePayloadRate(
      ModuleRTPUtility::Payload* payload,
      const uint32_t rate) const {
    payload->typeSpecific.Audio.rate = rate;
  }

  ModuleRTPUtility::Payload* CreatePayloadType(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payloadType,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) const {
    ModuleRTPUtility::Payload* payload = new ModuleRTPUtility::Payload;
    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
    payload->typeSpecific.Audio.frequency = frequency;
    payload->typeSpecific.Audio.channels = channels;
    payload->typeSpecific.Audio.rate = rate;
    payload->audio = true;
    return payload;
  }
};

class RTPPayloadVideoStrategy : public RTPPayloadStrategy {
 public:
  bool CodecsMustBeUnique() const { return false; }

  bool PayloadIsCompatible(
      const ModuleRTPUtility::Payload& payload,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) const {
    return !payload.audio;
  }

  void UpdatePayloadRate(
      ModuleRTPUtility::Payload* payload,
      const uint32_t rate) const {
    payload->typeSpecific.Video.maxRate = rate;
  }

  ModuleRTPUtility::Payload* CreatePayloadType(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payloadType,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) const {
    RtpVideoCodecTypes videoType = kRtpGenericVideo;
    if (ModuleRTPUtility::StringCompare(payloadName, "VP8", 3)) {
      videoType = kRtpVp8Video;
    } else if (ModuleRTPUtility::StringCompare(payloadName, "I420", 4)) {
      videoType = kRtpGenericVideo;
    } else if (ModuleRTPUtility::StringCompare(payloadName, "ULPFEC", 6)) {
      videoType = kRtpFecVideo;
    } else {
      videoType = kRtpGenericVideo;
    }
    ModuleRTPUtility::Payload* payload = new ModuleRTPUtility::Payload;

    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
    payload->typeSpecific.Video.videoCodecType = videoType;
    payload->typeSpecific.Video.maxRate = rate;
    payload->audio = false;
    return payload;
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
