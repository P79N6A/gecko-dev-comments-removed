









#include "webrtc/modules/video_coding/main/source/receiver.h"

#include <assert.h>

#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/encoded_frame.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/media_opt_util.h"
#include "webrtc/modules/video_coding/main/source/tick_time_base.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

VCMReceiver::VCMReceiver(VCMTiming* timing,
                         TickTimeBase* clock,
                         int32_t vcm_id,
                         int32_t receiver_id,
                         bool master)
    : crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      vcm_id_(vcm_id),
      clock_(clock),
      receiver_id_(receiver_id),
      master_(master),
      jitter_buffer_(clock_, vcm_id, receiver_id, master),
      timing_(timing),
      render_wait_event_(),
      state_(kPassive) {}

VCMReceiver::~VCMReceiver() {
  render_wait_event_.Set();
  delete crit_sect_;
}

void VCMReceiver::Reset() {
  CriticalSectionScoped cs(crit_sect_);
  if (!jitter_buffer_.Running()) {
    jitter_buffer_.Start();
  } else {
    jitter_buffer_.Flush();
  }
  render_wait_event_.Reset();
  if (master_) {
    state_ = kReceiving;
  } else {
    state_ = kPassive;
  }
}

int32_t VCMReceiver::Initialize() {
  CriticalSectionScoped cs(crit_sect_);
  Reset();
  if (!master_) {
    SetNackMode(kNoNack);
  }
  return VCM_OK;
}

void VCMReceiver::UpdateRtt(uint32_t rtt) {
  jitter_buffer_.UpdateRtt(rtt);
}

int32_t VCMReceiver::InsertPacket(const VCMPacket& packet, uint16_t frame_width,
                                  uint16_t frame_height) {
  
  VCMEncodedFrame* buffer = NULL;
  const int32_t error = jitter_buffer_.GetFrame(packet, buffer);
  if (error == VCM_OLD_PACKET_ERROR) {
    return VCM_OK;
  } else if (error != VCM_OK) {
    return error;
  }
  assert(buffer);
  {
    CriticalSectionScoped cs(crit_sect_);

    if (frame_width && frame_height) {
      buffer->SetEncodedSize(static_cast<uint32_t>(frame_width),
                             static_cast<uint32_t>(frame_height));
    }

    if (master_) {
      
      
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                   VCMId(vcm_id_, receiver_id_),
                   "Packet seq_no %u of frame %u at %u",
                   packet.seqNum, packet.timestamp,
                   MaskWord64ToUWord32(clock_->MillisecondTimestamp()));
    }

    const int64_t now_ms = clock_->MillisecondTimestamp();

    int64_t render_time_ms = timing_->RenderTimeMs(packet.timestamp, now_ms);

    if (render_time_ms < 0) {
      
      
      jitter_buffer_.Flush();
      timing_->Reset(clock_->MillisecondTimestamp());
      return VCM_FLUSH_INDICATOR;
    } else if (render_time_ms < now_ms - kMaxVideoDelayMs) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                   VCMId(vcm_id_, receiver_id_),
                   "This frame should have been rendered more than %u ms ago."
                   "Flushing jitter buffer and resetting timing.",
                   kMaxVideoDelayMs);
      jitter_buffer_.Flush();
      timing_->Reset(clock_->MillisecondTimestamp());
      return VCM_FLUSH_INDICATOR;
    } else if (timing_->TargetVideoDelay() > kMaxVideoDelayMs) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                   VCMId(vcm_id_, receiver_id_),
                   "More than %u ms target delay. Flushing jitter buffer and"
                   "resetting timing.", kMaxVideoDelayMs);
      jitter_buffer_.Flush();
      timing_->Reset(clock_->MillisecondTimestamp());
      return VCM_FLUSH_INDICATOR;
    }

    
    if (buffer->Length() == 0) {
      const int64_t now_ms = clock_->MillisecondTimestamp();
      if (master_) {
        
        
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(vcm_id_, receiver_id_),
                     "First packet of frame %u at %u", packet.timestamp,
                     MaskWord64ToUWord32(now_ms));
      }
      render_time_ms = timing_->RenderTimeMs(packet.timestamp, now_ms);
      if (render_time_ms >= 0) {
        buffer->SetRenderTime(render_time_ms);
      } else {
        buffer->SetRenderTime(now_ms);
      }
    }

    
    const VCMFrameBufferEnum
    ret = jitter_buffer_.InsertPacket(buffer, packet);
    if (ret == kFlushIndicator) {
      return VCM_FLUSH_INDICATOR;
    } else if (ret < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding,
                   VCMId(vcm_id_, receiver_id_),
                   "Error inserting packet seq_no=%u, time_stamp=%u",
                   packet.seqNum, packet.timestamp);
      return VCM_JITTER_BUFFER_ERROR;
    }
  }
  return VCM_OK;
}

VCMEncodedFrame* VCMReceiver::FrameForDecoding(
    uint16_t max_wait_time_ms,
    int64_t& next_render_time_ms,
    bool render_timing,
    VCMReceiver* dual_receiver) {
  
  
  FrameType incoming_frame_type = kVideoFrameDelta;
  next_render_time_ms = -1;
  const int64_t start_time_ms = clock_->MillisecondTimestamp();
  int64_t ret = jitter_buffer_.NextTimestamp(max_wait_time_ms,
                                             &incoming_frame_type,
                                             &next_render_time_ms);
  if (ret < 0) {
    
    return NULL;
  }
  const uint32_t time_stamp = static_cast<uint32_t>(ret);

  
  timing_->SetRequiredDelay(jitter_buffer_.EstimatedJitterMs());
  timing_->UpdateCurrentDelay(time_stamp);

  const int32_t temp_wait_time = max_wait_time_ms -
      static_cast<int32_t>(clock_->MillisecondTimestamp() - start_time_ms);
  uint16_t new_max_wait_time = static_cast<uint16_t>(VCM_MAX(temp_wait_time,
                                                             0));

  VCMEncodedFrame* frame = NULL;

  if (render_timing) {
    frame = FrameForDecoding(new_max_wait_time, next_render_time_ms,
                             dual_receiver);
  } else {
    frame = FrameForRendering(new_max_wait_time, next_render_time_ms,
                              dual_receiver);
  }

  if (frame != NULL) {
    bool retransmitted = false;
    const int64_t last_packet_time_ms =
      jitter_buffer_.LastPacketTime(frame, &retransmitted);
    if (last_packet_time_ms >= 0 && !retransmitted) {
      
      
      
      timing_->IncomingTimestamp(time_stamp, last_packet_time_ms);
    }
    if (dual_receiver != NULL) {
      dual_receiver->UpdateState(*frame);
    }
  }
  return frame;
}

VCMEncodedFrame* VCMReceiver::FrameForDecoding(
    uint16_t max_wait_time_ms,
    int64_t next_render_time_ms,
    VCMReceiver* dual_receiver) {
  
  uint32_t wait_time_ms = timing_->MaxWaitingTime(
      next_render_time_ms, clock_->MillisecondTimestamp());

  
  VCMEncodedFrame* frame = jitter_buffer_.GetCompleteFrameForDecoding(0);

  if (frame == NULL && max_wait_time_ms == 0 && wait_time_ms > 0) {
    
    
    
    return NULL;
  }

  if (frame == NULL && VCM_MIN(wait_time_ms, max_wait_time_ms) == 0) {
    
    const bool dual_receiver_enabled_and_passive = (dual_receiver != NULL &&
        dual_receiver->State() == kPassive &&
        dual_receiver->NackMode() == kNackInfinite);
    if (dual_receiver_enabled_and_passive &&
        !jitter_buffer_.CompleteSequenceWithNextFrame()) {
      
      dual_receiver->CopyJitterBufferStateFromReceiver(*this);
      frame = jitter_buffer_.GetFrameForDecoding();
      assert(frame);
    } else {
      frame = jitter_buffer_.GetFrameForDecoding();
    }
  }
  if (frame == NULL) {
    
    frame = jitter_buffer_.GetCompleteFrameForDecoding(max_wait_time_ms);
  }
  if (frame == NULL) {
    
    if (timing_->MaxWaitingTime(next_render_time_ms,
                                clock_->MillisecondTimestamp()) > 0) {
      
      return NULL;
    }

    
    const bool dual_receiver_enabled_and_passive = (dual_receiver != NULL &&
        dual_receiver->State() == kPassive &&
        dual_receiver->NackMode() == kNackInfinite);
    if (dual_receiver_enabled_and_passive &&
        !jitter_buffer_.CompleteSequenceWithNextFrame()) {
      
      dual_receiver->CopyJitterBufferStateFromReceiver(*this);
    }

    frame = jitter_buffer_.GetFrameForDecoding();
  }
  return frame;
}

VCMEncodedFrame* VCMReceiver::FrameForRendering(uint16_t max_wait_time_ms,
                                                int64_t next_render_time_ms,
                                                VCMReceiver* dual_receiver) {
  
  
  
  
  
  uint32_t wait_time_ms = timing_->MaxWaitingTime(
      next_render_time_ms, clock_->MillisecondTimestamp());
  if (max_wait_time_ms < wait_time_ms) {
    
    
    return NULL;
  }
  
  render_wait_event_.Wait(wait_time_ms);

  
  VCMEncodedFrame* frame = jitter_buffer_.GetCompleteFrameForDecoding(0);

  if (frame == NULL) {
    
    const bool dual_receiver_enabled_and_passive = (dual_receiver != NULL &&
        dual_receiver->State() == kPassive &&
        dual_receiver->NackMode() == kNackInfinite);
    if (dual_receiver_enabled_and_passive &&
        !jitter_buffer_.CompleteSequenceWithNextFrame()) {
      
      dual_receiver->CopyJitterBufferStateFromReceiver(*this);
    }

    frame = jitter_buffer_.GetFrameForDecoding();
  }
  return frame;
}

void VCMReceiver::ReleaseFrame(VCMEncodedFrame* frame) {
  jitter_buffer_.ReleaseFrame(frame);
}

void VCMReceiver::ReceiveStatistics(uint32_t* bitrate,
                                    uint32_t* framerate) {
  assert(bitrate);
  assert(framerate);
  jitter_buffer_.IncomingRateStatistics(framerate, bitrate);
  *bitrate /= 1000;  
}

void VCMReceiver::ReceivedFrameCount(VCMFrameCount* frame_count) const {
  assert(frame_count);
  jitter_buffer_.FrameStatistics(&frame_count->numDeltaFrames,
                                 &frame_count->numKeyFrames);
}

uint32_t VCMReceiver::DiscardedPackets() const {
  return jitter_buffer_.num_discarded_packets();
}

void VCMReceiver::SetNackMode(VCMNackMode nackMode) {
  CriticalSectionScoped cs(crit_sect_);
  
  jitter_buffer_.SetNackMode(nackMode, kLowRttNackMs, -1);
  if (!master_) {
    state_ = kPassive;  
  }
}

VCMNackMode VCMReceiver::NackMode() const {
  CriticalSectionScoped cs(crit_sect_);
  return jitter_buffer_.nack_mode();
}

VCMNackStatus VCMReceiver::NackList(uint16_t* nack_list,
                                    uint16_t* size) {
  bool extended = false;
  uint16_t nack_list_size = 0;
  uint16_t* internal_nack_list = jitter_buffer_.CreateNackList(&nack_list_size,
                                                               &extended);
  if (internal_nack_list == NULL && nack_list_size == 0xffff) {
    
    *size = 0;
    return kNackKeyFrameRequest;
  }
  if (nack_list_size > *size) {
    *size = nack_list_size;
    return kNackNeedMoreMemory;
  }
  if (internal_nack_list != NULL && nack_list_size > 0) {
    memcpy(nack_list, internal_nack_list, nack_list_size * sizeof(uint16_t));
  }
  *size = nack_list_size;
  return kNackOk;
}



bool VCMReceiver::DualDecoderCaughtUp(VCMEncodedFrame* dual_frame,
                                      VCMReceiver& dual_receiver) const {
  if (dual_frame == NULL) {
    return false;
  }
  if (jitter_buffer_.LastDecodedTimestamp() == dual_frame->TimeStamp()) {
    dual_receiver.UpdateState(kWaitForPrimaryDecode);
    return true;
  }
  return false;
}

void VCMReceiver::CopyJitterBufferStateFromReceiver(
    const VCMReceiver& receiver) {
  jitter_buffer_.CopyFrom(receiver.jitter_buffer_);
}

VCMReceiverState VCMReceiver::State() const {
  CriticalSectionScoped cs(crit_sect_);
  return state_;
}

void VCMReceiver::UpdateState(VCMReceiverState new_state) {
  CriticalSectionScoped cs(crit_sect_);
  assert(!(state_ == kPassive && new_state == kWaitForPrimaryDecode));
  state_ = new_state;
}

void VCMReceiver::UpdateState(const VCMEncodedFrame& frame) {
  if (jitter_buffer_.nack_mode() == kNoNack) {
    
    return;
  }
  
  if (frame.Complete() && frame.FrameType() == kVideoFrameKey) {
    UpdateState(kPassive);
  }
  if (State() == kWaitForPrimaryDecode &&
      frame.Complete() && !frame.MissingFrame()) {
    UpdateState(kPassive);
  }
  if (frame.MissingFrame() || !frame.Complete()) {
    
    UpdateState(kReceiving);
  }
}
}  
