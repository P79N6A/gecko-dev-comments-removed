









#include "video_engine/vie_remb.h"

#include <algorithm>
#include <cassert>

#include "modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "modules/utility/interface/process_thread.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/tick_util.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

const int kRembTimeOutThresholdMs = 2000;
const int kRembSendIntervallMs = 1000;
const unsigned int kRembMinimumBitrateKbps = 50;


const unsigned int kSendThresholdPercent = 97;

VieRemb::VieRemb()
    : list_crit_(CriticalSectionWrapper::CreateCriticalSection()),
      last_remb_time_(TickTime::MillisecondTimestamp()),
      last_send_bitrate_(0),
      bitrate_(0) {}

VieRemb::~VieRemb() {}

void VieRemb::AddReceiveChannel(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);
  WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
               "VieRemb::AddReceiveChannel(%p)", rtp_rtcp);

  CriticalSectionScoped cs(list_crit_.get());
  if (std::find(receive_modules_.begin(), receive_modules_.end(), rtp_rtcp) !=
      receive_modules_.end())
    return;

  WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1, "AddRembChannel");
  
  
  receive_modules_.push_back(rtp_rtcp);
}

void VieRemb::RemoveReceiveChannel(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);
  WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
               "VieRemb::RemoveReceiveChannel(%p)", rtp_rtcp);

  CriticalSectionScoped cs(list_crit_.get());
  for (RtpModules::iterator it = receive_modules_.begin();
       it != receive_modules_.end(); ++it) {
    if ((*it) == rtp_rtcp) {
      receive_modules_.erase(it);
      break;
    }
  }
}

void VieRemb::AddRembSender(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);
  WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
               "VieRemb::AddRembSender(%p)", rtp_rtcp);

  CriticalSectionScoped cs(list_crit_.get());

  
  if (std::find(rtcp_sender_.begin(), rtcp_sender_.end(), rtp_rtcp) !=
      rtcp_sender_.end())
    return;
  rtcp_sender_.push_back(rtp_rtcp);
}

void VieRemb::RemoveRembSender(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);
  WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
               "VieRemb::RemoveRembSender(%p)", rtp_rtcp);

  CriticalSectionScoped cs(list_crit_.get());
  for (RtpModules::iterator it = rtcp_sender_.begin();
       it != rtcp_sender_.end(); ++it) {
    if ((*it) == rtp_rtcp) {
      rtcp_sender_.erase(it);
      return;
    }
  }
}

bool VieRemb::InUse() const {
  CriticalSectionScoped cs(list_crit_.get());
  if (receive_modules_.empty() && rtcp_sender_.empty())
    return false;
  else
    return true;
}

void VieRemb::OnReceiveBitrateChanged(std::vector<unsigned int>* ssrcs,
                                      unsigned int bitrate) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, -1,
               "VieRemb::UpdateBitrateEstimate(bitrate: %u)", bitrate);
  assert(ssrcs);
  list_crit_->Enter();
  
  
  if (last_send_bitrate_ > 0) {
    unsigned int new_remb_bitrate = last_send_bitrate_ - bitrate_ + bitrate;

    if (new_remb_bitrate < kSendThresholdPercent * last_send_bitrate_ / 100) {
      
      
      last_remb_time_ = TickTime::MillisecondTimestamp() - kRembSendIntervallMs;
    }
  }
  bitrate_ = bitrate;
  
  
  ssrcs_.resize(ssrcs->size());
  std::copy(ssrcs->begin(), ssrcs->end(), ssrcs_.begin());

  
  int64_t now = TickTime::MillisecondTimestamp();

  if (now - last_remb_time_ < kRembSendIntervallMs) {
    list_crit_->Leave();
    return;
  }
  last_remb_time_ = now;

  if (ssrcs_.empty() || receive_modules_.empty()) {
    list_crit_->Leave();
    return;
  }

  
  RtpRtcp* sender = NULL;
  if (!rtcp_sender_.empty()) {
    sender = rtcp_sender_.front();
  } else {
    sender = receive_modules_.front();
  }
  last_send_bitrate_ = bitrate_;

  
  if (last_send_bitrate_ < kRembMinimumBitrateKbps) {
    last_send_bitrate_ = kRembMinimumBitrateKbps;
  }
  
  int ssrcs_length = ssrcs_.size();
  unsigned int* ssrcs_copy = new unsigned int[ssrcs_length];
  for (int i = 0; i < ssrcs_length; ++i) {
    ssrcs_copy[i] = ssrcs_[i];
  }
  list_crit_->Leave();

  if (sender) {
    
    sender->SetREMBData(bitrate_, ssrcs_length, ssrcs_copy);
  }
  delete [] ssrcs_copy;
}

}  
