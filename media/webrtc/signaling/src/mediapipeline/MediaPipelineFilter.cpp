








#include "logging.h"

#include "MediaPipelineFilter.h"

#include "webrtc/modules/interface/module_common_types.h"


MOZ_MTLOG_MODULE("mediapipeline")

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

MediaPipelineFilter::Result
MediaPipelineFilter::FilterRTCP(const unsigned char* data,
                                size_t len) const {
  if (len < FIRST_SSRC_OFFSET) {
    return FAIL;
  }

  uint8_t payload_type = data[PT_OFFSET];

  switch (payload_type) {
    case SENDER_REPORT_T:
      return CheckRtcpReport(data, len, RECEIVER_REPORT_START_SR) ? PASS : FAIL;
    case RECEIVER_REPORT_T:
      return CheckRtcpReport(data, len, SENDER_REPORT_START_RR) ? PASS : FAIL;
    default:
      return UNSUPPORTED;
  }

  return UNSUPPORTED;
}

bool MediaPipelineFilter::CheckRtcpSsrc(const unsigned char* data,
                                        size_t len,
                                        size_t ssrc_offset,
                                        uint8_t flags) const {
  if (ssrc_offset + 4 > len) {
    return false;
  }

  uint32_t ssrc = 0;
  ssrc += (uint32_t)data[ssrc_offset++] << 24;
  ssrc += (uint32_t)data[ssrc_offset++] << 16;
  ssrc += (uint32_t)data[ssrc_offset++] << 8;
  ssrc += (uint32_t)data[ssrc_offset++];

  if (flags | MAYBE_LOCAL_SSRC) {
    if (local_ssrc_set_.count(ssrc)) {
      return true;
    }
  }

  if (flags | MAYBE_REMOTE_SSRC) {
    if (remote_ssrc_set_.count(ssrc)) {
      return true;
    }
  }
  return false;
}

static uint8_t GetCount(const unsigned char* data, size_t len) {
  
  
  return data[0] & 0x1F;
}

bool MediaPipelineFilter::CheckRtcpReport(const unsigned char* data,
                                          size_t len,
                                          size_t first_rr_offset) const {
  bool remote_ssrc_matched = CheckRtcpSsrc(data,
                                           len,
                                           FIRST_SSRC_OFFSET,
                                           MAYBE_REMOTE_SSRC);

  uint8_t rr_count = GetCount(data, len);

  
  
  
  bool ssrcs_must_match = remote_ssrc_matched;
  bool ssrcs_must_not_match = false;

  for (uint8_t rr_num = 0; rr_num < rr_count; ++rr_num) {
    size_t ssrc_offset = first_rr_offset + (rr_num * RECEIVER_REPORT_SIZE);

    if (!CheckRtcpSsrc(data, len, ssrc_offset, MAYBE_LOCAL_SSRC)) {
      ssrcs_must_not_match = true;
      if (ssrcs_must_match) {
        break;
      }
    } else {
      ssrcs_must_match = true;
      if (ssrcs_must_not_match) {
        break;
      }
    }
  }

  if (ssrcs_must_match && ssrcs_must_not_match) {
    MOZ_MTLOG(ML_ERROR, "Received an RTCP packet with SSRCs from "
              "multiple m-lines! This is broken.");
    return false;
  }

  
  return ssrcs_must_match;
}

} 


