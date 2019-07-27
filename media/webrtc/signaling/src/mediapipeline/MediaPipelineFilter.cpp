








#include "MediaPipelineFilter.h"

#include "webrtc/modules/interface/module_common_types.h"

namespace mozilla {

MediaPipelineFilter::MediaPipelineFilter() : correlator_(0) {
}

bool MediaPipelineFilter::Filter(const webrtc::RTPHeader& header,
                                 uint32_t correlator) {
  if (correlator) {
    
    
    if (correlator == correlator_) {
      AddRemoteSSRC(header.ssrc);
      return true;
    } else {
      
      
      remote_ssrc_set_.erase(header.ssrc);
      return false;
    }
  }

  if (remote_ssrc_set_.count(header.ssrc)) {
    return true;
  }

  
  if (payload_type_set_.count(header.payloadType)) {
    
    
    AddRemoteSSRC(header.ssrc);
    return true;
  }

  return false;
}

void MediaPipelineFilter::AddLocalSSRC(uint32_t ssrc) {
  local_ssrc_set_.insert(ssrc);
}

void MediaPipelineFilter::AddRemoteSSRC(uint32_t ssrc) {
  remote_ssrc_set_.insert(ssrc);
}

void MediaPipelineFilter::AddUniquePT(uint8_t payload_type) {
  payload_type_set_.insert(payload_type);
}

void MediaPipelineFilter::SetCorrelator(uint32_t correlator) {
  correlator_ = correlator;
}

void MediaPipelineFilter::Update(const MediaPipelineFilter& filter_update) {
  
  
  
  if (!filter_update.remote_ssrc_set_.empty()) {
    remote_ssrc_set_ = filter_update.remote_ssrc_set_;
  }

  local_ssrc_set_ = filter_update.local_ssrc_set_;
  payload_type_set_ = filter_update.payload_type_set_;
  correlator_ = filter_update.correlator_;
}

bool
MediaPipelineFilter::FilterSenderReport(const unsigned char* data,
                                        size_t len) const {
  if (len < FIRST_SSRC_OFFSET) {
    return false;
  }

  uint8_t payload_type = data[PT_OFFSET];

  if (payload_type != SENDER_REPORT_T) {
    return false;
  }

  uint32_t ssrc = 0;
  ssrc += (uint32_t)data[FIRST_SSRC_OFFSET] << 24;
  ssrc += (uint32_t)data[FIRST_SSRC_OFFSET + 1] << 16;
  ssrc += (uint32_t)data[FIRST_SSRC_OFFSET + 2] << 8;
  ssrc += (uint32_t)data[FIRST_SSRC_OFFSET + 3];

  return !!remote_ssrc_set_.count(ssrc);
}

} 

