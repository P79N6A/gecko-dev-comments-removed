









#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_audio.h"

#include <assert.h>  
#include <math.h>   
#include <string.h>  

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {
RTPReceiverStrategy* RTPReceiverStrategy::CreateAudioStrategy(
    int32_t id, RtpData* data_callback,
    RtpAudioFeedback* incoming_messages_callback) {
  return new RTPReceiverAudio(id, data_callback, incoming_messages_callback);
}

RTPReceiverAudio::RTPReceiverAudio(const int32_t id,
                                   RtpData* data_callback,
                                   RtpAudioFeedback* incoming_messages_callback)
    : RTPReceiverStrategy(data_callback),
      TelephoneEventHandler(),
      id_(id),
      last_received_frequency_(8000),
      telephone_event_forward_to_decoder_(false),
      telephone_event_payload_type_(-1),
      cng_nb_payload_type_(-1),
      cng_wb_payload_type_(-1),
      cng_swb_payload_type_(-1),
      cng_fb_payload_type_(-1),
      cng_payload_type_(-1),
      g722_payload_type_(-1),
      last_received_g722_(false),
      num_energy_(0),
      current_remote_energy_(),
      cb_audio_feedback_(incoming_messages_callback) {
  last_payload_.Audio.channels = 1;
  memset(current_remote_energy_, 0, sizeof(current_remote_energy_));
}


void RTPReceiverAudio::SetTelephoneEventForwardToDecoder(
    bool forward_to_decoder) {
  CriticalSectionScoped lock(crit_sect_.get());
  telephone_event_forward_to_decoder_ = forward_to_decoder;
}


bool RTPReceiverAudio::TelephoneEventForwardToDecoder() const {
  CriticalSectionScoped lock(crit_sect_.get());
  return telephone_event_forward_to_decoder_;
}

bool RTPReceiverAudio::TelephoneEventPayloadType(
    int8_t payload_type) const {
  CriticalSectionScoped lock(crit_sect_.get());
  return (telephone_event_payload_type_ == payload_type) ? true : false;
}

bool RTPReceiverAudio::CNGPayloadType(int8_t payload_type,
                                      uint32_t* frequency,
                                      bool* cng_payload_type_has_changed) {
  CriticalSectionScoped lock(crit_sect_.get());
  *cng_payload_type_has_changed = false;

  
  if (cng_nb_payload_type_ == payload_type) {
    *frequency = 8000;
    if (cng_payload_type_ != -1 && cng_payload_type_ != cng_nb_payload_type_)
      *cng_payload_type_has_changed = true;

    cng_payload_type_ = cng_nb_payload_type_;
    return true;
  } else if (cng_wb_payload_type_ == payload_type) {
    
    if (last_received_g722_) {
      *frequency = 8000;
    } else {
      *frequency = 16000;
    }
    if (cng_payload_type_ != -1 && cng_payload_type_ != cng_wb_payload_type_)
      *cng_payload_type_has_changed = true;
    cng_payload_type_ = cng_wb_payload_type_;
    return true;
  } else if (cng_swb_payload_type_ == payload_type) {
    *frequency = 32000;
    if ((cng_payload_type_ != -1) &&
        (cng_payload_type_ != cng_swb_payload_type_))
      *cng_payload_type_has_changed = true;
    cng_payload_type_ = cng_swb_payload_type_;
    return true;
  } else if (cng_fb_payload_type_ == payload_type) {
    *frequency = 48000;
    if (cng_payload_type_ != -1 && cng_payload_type_ != cng_fb_payload_type_)
      *cng_payload_type_has_changed = true;
    cng_payload_type_ = cng_fb_payload_type_;
    return true;
  } else {
    
    if (g722_payload_type_ == payload_type) {
      last_received_g722_ = true;
    } else {
      last_received_g722_ = false;
    }
  }
  return false;
}

bool RTPReceiverAudio::ShouldReportCsrcChanges(uint8_t payload_type) const {
  
  return !TelephoneEventPayloadType(payload_type);
}

































int32_t RTPReceiverAudio::OnNewPayloadTypeCreated(
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    int8_t payload_type,
    uint32_t frequency) {
  CriticalSectionScoped lock(crit_sect_.get());

  if (ModuleRTPUtility::StringCompare(payload_name, "telephone-event", 15)) {
    telephone_event_payload_type_ = payload_type;
  }
  if (ModuleRTPUtility::StringCompare(payload_name, "cn", 2)) {
    
    if (frequency == 8000) {
      cng_nb_payload_type_ = payload_type;
    } else if (frequency == 16000) {
      cng_wb_payload_type_ = payload_type;
    } else if (frequency == 32000) {
      cng_swb_payload_type_ = payload_type;
    } else if (frequency == 48000) {
      cng_fb_payload_type_ = payload_type;
    } else {
      assert(false);
      return -1;
    }
  }
  return 0;
}

int32_t RTPReceiverAudio::ParseRtpPacket(WebRtcRTPHeader* rtp_header,
                                         const PayloadUnion& specific_payload,
                                         bool is_red,
                                         const uint8_t* payload,
                                         uint16_t payload_length,
                                         int64_t timestamp_ms,
                                         bool is_first_packet) {
  TRACE_EVENT2("webrtc_rtp", "Audio::ParseRtp",
               "seqnum", rtp_header->header.sequenceNumber,
               "timestamp", rtp_header->header.timestamp);
  rtp_header->type.Audio.numEnergy = rtp_header->header.numCSRCs;
  num_energy_ = rtp_header->type.Audio.numEnergy;
  if (rtp_header->type.Audio.numEnergy > 0 &&
      rtp_header->type.Audio.numEnergy <= kRtpCsrcSize) {
    memcpy(current_remote_energy_,
           rtp_header->type.Audio.arrOfEnergy,
           rtp_header->type.Audio.numEnergy);
  }

  return ParseAudioCodecSpecific(rtp_header,
                                 payload,
                                 payload_length,
                                 specific_payload.Audio,
                                 is_red);
}

int RTPReceiverAudio::GetPayloadTypeFrequency() const {
  CriticalSectionScoped lock(crit_sect_.get());
  if (last_received_g722_) {
    return 8000;
  }
  return last_received_frequency_;
}

RTPAliveType RTPReceiverAudio::ProcessDeadOrAlive(
    uint16_t last_payload_length) const {

  
  
  if (last_payload_length < 10) {  
    return kRtpNoRtp;
  } else {
    return kRtpDead;
  }
}

void RTPReceiverAudio::CheckPayloadChanged(int8_t payload_type,
                                           PayloadUnion* specific_payload,
                                           bool* should_reset_statistics,
                                           bool* should_discard_changes) {
  *should_discard_changes = false;
  *should_reset_statistics = false;

  if (TelephoneEventPayloadType(payload_type)) {
    
    *should_discard_changes = true;
    return;
  }
  
  bool cng_payload_type_has_changed = false;
  bool is_cng_payload_type = CNGPayloadType(payload_type,
                                            &specific_payload->Audio.frequency,
                                            &cng_payload_type_has_changed);

  *should_reset_statistics = cng_payload_type_has_changed;

  if (is_cng_payload_type) {
    
    *should_discard_changes = true;
    return;
  }
}

int RTPReceiverAudio::Energy(uint8_t array_of_energy[kRtpCsrcSize]) const {
  CriticalSectionScoped cs(crit_sect_.get());

  assert(num_energy_ <= kRtpCsrcSize);

  if (num_energy_ > 0) {
    memcpy(array_of_energy, current_remote_energy_,
           sizeof(uint8_t) * num_energy_);
  }
  return num_energy_;
}

int32_t RTPReceiverAudio::InvokeOnInitializeDecoder(
    RtpFeedback* callback,
    int32_t id,
    int8_t payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const PayloadUnion& specific_payload) const {
  if (-1 == callback->OnInitializeDecoder(id,
                                          payload_type,
                                          payload_name,
                                          specific_payload.Audio.frequency,
                                          specific_payload.Audio.channels,
                                          specific_payload.Audio.rate)) {
    WEBRTC_TRACE(kTraceError,
                 kTraceRtpRtcp,
                 id,
                 "Failed to create video decoder for payload type:%d",
                 payload_type);
    return -1;
  }
  return 0;
}


int32_t RTPReceiverAudio::ParseAudioCodecSpecific(
    WebRtcRTPHeader* rtp_header,
    const uint8_t* payload_data,
    uint16_t payload_length,
    const AudioPayload& audio_specific,
    bool is_red) {

  if (payload_length == 0) {
    return 0;
  }

  bool telephone_event_packet =
      TelephoneEventPayloadType(rtp_header->header.payloadType);
  if (telephone_event_packet) {
    CriticalSectionScoped lock(crit_sect_.get());

    
    
    
    
    
    
    
    if (payload_length % 4 != 0) {
      return -1;
    }
    uint8_t number_of_events = payload_length / 4;

    
    if (number_of_events >= MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS) {
      number_of_events = MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS;
    }
    for (int n = 0; n < number_of_events; ++n) {
      bool end = (payload_data[(4 * n) + 1] & 0x80) ? true : false;

      std::set<uint8_t>::iterator event =
          telephone_event_reported_.find(payload_data[4 * n]);

      if (event != telephone_event_reported_.end()) {
        
        if (end) {
          telephone_event_reported_.erase(payload_data[4 * n]);
        }
      } else {
        if (end) {
          
        } else {
          telephone_event_reported_.insert(payload_data[4 * n]);
        }
      }
    }

    
    

    
  }

  {
    CriticalSectionScoped lock(crit_sect_.get());

    if (!telephone_event_packet) {
      last_received_frequency_ = audio_specific.frequency;
    }

    
    uint32_t ignored;
    bool also_ignored;
    if (CNGPayloadType(rtp_header->header.payloadType,
                       &ignored,
                       &also_ignored)) {
      rtp_header->type.Audio.isCNG = true;
      rtp_header->frameType = kAudioFrameCN;
    } else {
      rtp_header->frameType = kAudioFrameSpeech;
      rtp_header->type.Audio.isCNG = false;
    }

    
    if (telephone_event_packet) {
      if (!telephone_event_forward_to_decoder_) {
        
        return 0;
      }
      std::set<uint8_t>::iterator first =
          telephone_event_reported_.begin();
      if (first != telephone_event_reported_.end() && *first > 15) {
        
        return 0;
      }
    }
  }
  
  if (is_red && !(payload_data[0] & 0x80)) {
    
    rtp_header->header.payloadType = payload_data[0];

    
    return data_callback_->OnReceivedPayloadData(
        payload_data + 1, payload_length - 1, rtp_header);
  }

  rtp_header->type.Audio.channel = audio_specific.channels;
  return data_callback_->OnReceivedPayloadData(
      payload_data, payload_length, rtp_header);
}
}  
