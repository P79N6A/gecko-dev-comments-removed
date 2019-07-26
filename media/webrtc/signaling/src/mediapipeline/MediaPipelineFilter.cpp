








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

bool MediaPipelineFilter::FilterRTCP(uint32_t ssrc) {
  return remote_ssrc_set_.count(ssrc) != 0;
}

bool MediaPipelineFilter::FilterRTCPReceiverReport(uint32_t ssrc) {
  return local_ssrc_set_.count(ssrc) != 0;
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

void MediaPipelineFilter::IncorporateRemoteDescription(
    const MediaPipelineFilter& remote_filter) {
  
  
  if (!remote_filter.remote_ssrc_set_.empty()) {
    remote_ssrc_set_ = remote_filter.remote_ssrc_set_;
  }

  
  
}

} 


