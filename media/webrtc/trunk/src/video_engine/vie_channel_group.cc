









#include "video_engine/vie_channel_group.h"

#include "modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "video_engine/vie_channel.h"
#include "video_engine/vie_encoder.h"
#include "video_engine/vie_remb.h"

namespace webrtc {

ChannelGroup::ChannelGroup(ProcessThread* process_thread)
    : remb_(new VieRemb(process_thread)) {}

ChannelGroup::~ChannelGroup() {
  assert(channels_.empty());
  assert(!remb_->InUse());
}
void ChannelGroup::AddChannel(int channel_id) {
  channels_.insert(channel_id);
}

void ChannelGroup::RemoveChannel(int channel_id) {
  channels_.erase(channel_id);
}

bool ChannelGroup::HasChannel(int channel_id) {
  return channels_.find(channel_id) != channels_.end();
}

bool ChannelGroup::Empty() {
  return channels_.empty();
}

bool ChannelGroup::SetChannelRembStatus(int channel_id,
                                        bool sender,
                                        bool receiver,
                                        ViEChannel* channel,
                                        ViEEncoder* encoder) {
  
  if (sender || receiver) {
    if (!channel->EnableRemb(true)) {
      return false;
    }
  } else if (channel) {
    channel->EnableRemb(false);
  }

  
  RtpRtcp* rtp_module = channel->rtp_rtcp();
  if (sender) {
    remb_->AddRembSender(rtp_module);
    remb_->AddSendChannel(encoder->SendRtpRtcpModule());
  } else {
    remb_->RemoveRembSender(rtp_module);
    remb_->RemoveSendChannel(encoder->SendRtpRtcpModule());
  }
  if (receiver) {
    remb_->AddReceiveChannel(rtp_module);
  } else {
    remb_->RemoveReceiveChannel(rtp_module);
  }
  if (sender || receiver) {
    rtp_module->SetRemoteBitrateObserver(remb_.get());
  } else {
    rtp_module->SetRemoteBitrateObserver(NULL);
  }
  return true;
}

}  
