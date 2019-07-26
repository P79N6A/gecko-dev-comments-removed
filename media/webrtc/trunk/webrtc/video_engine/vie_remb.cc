









#include "webrtc/video_engine/vie_remb.h"

#include <assert.h>

#include <algorithm>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/utility/interface/process_thread.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

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

void VieRemb::OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs,
                                      unsigned int bitrate) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, -1,
               "VieRemb::UpdateBitrateEstimate(bitrate: %u)", bitrate);
  list_crit_->Enter();
  
  
  if (last_send_bitrate_ > 0) {
    unsigned int new_remb_bitrate = last_send_bitrate_ - bitrate_ + bitrate;

    if (new_remb_bitrate < kSendThresholdPercent * last_send_bitrate_ / 100) {
      
      
      last_remb_time_ = TickTime::MillisecondTimestamp() - kRembSendIntervallMs;
    }
  }
  bitrate_ = bitrate;

  
  int64_t now = TickTime::MillisecondTimestamp();

  if (now - last_remb_time_ < kRembSendIntervallMs) {
    list_crit_->Leave();
    return;
  }
  last_remb_time_ = now;

  if (ssrcs.empty() || receive_modules_.empty()) {
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

  list_crit_->Leave();

  if (sender) {
    
    sender->SetREMBData(bitrate_, ssrcs.size(), &ssrcs[0]);
  }
}

}  
