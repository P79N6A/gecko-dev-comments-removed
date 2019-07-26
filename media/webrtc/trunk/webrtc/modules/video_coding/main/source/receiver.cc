









#include "webrtc/modules/video_coding/main/source/receiver.h"

#include <assert.h>

#include "webrtc/modules/video_coding/main/source/encoded_frame.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/media_opt_util.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {

enum { kMaxReceiverDelayMs = 10000 };

VCMReceiver::VCMReceiver(VCMTiming* timing,
                         Clock* clock,
                         EventFactory* event_factory,
                         int32_t vcm_id,
                         int32_t receiver_id,
                         bool master)
    : crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      vcm_id_(vcm_id),
      clock_(clock),
      receiver_id_(receiver_id),
      master_(master),
      jitter_buffer_(clock_, event_factory, vcm_id, receiver_id, master),
      timing_(timing),
      render_wait_event_(event_factory->CreateEvent()),
      state_(kPassive),
      max_video_delay_ms_(kMaxVideoDelayMs) {}

VCMReceiver::~VCMReceiver() {
  render_wait_event_->Set();
  delete crit_sect_;
}

void VCMReceiver::Reset() {
  CriticalSectionScoped cs(crit_sect_);
  if (!jitter_buffer_.Running()) {
    jitter_buffer_.Start();
  } else {
    jitter_buffer_.Flush();
  }
  render_wait_event_->Reset();
  if (master_) {
    state_ = kReceiving;
  } else {
    state_ = kPassive;
  }
}

int32_t VCMReceiver::Initialize() {
  Reset();
  CriticalSectionScoped cs(crit_sect_);
  if (!master_) {
    SetNackMode(kNoNack, -1, -1);
  }
  return VCM_OK;
}

void VCMReceiver::UpdateRtt(uint32_t rtt) {
  jitter_buffer_.UpdateRtt(rtt);
}

int32_t VCMReceiver::InsertPacket(const VCMPacket& packet,
                                  uint16_t frame_width,
                                  uint16_t frame_height) {
  if (packet.frameType == kVideoFrameKey) {
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Inserting key frame packet seqnum=%u, timestamp=%u",
                 packet.seqNum, packet.timestamp);
  }

  
  
  bool retransmitted = false;
  const VCMFrameBufferEnum ret = jitter_buffer_.InsertPacket(packet,
                                                             &retransmitted);
  if (ret == kOldPacket) {
    return VCM_OK;
  } else if (ret == kFlushIndicator) {
    return VCM_FLUSH_INDICATOR;
  } else if (ret < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Error inserting packet seqnum=%u, timestamp=%u",
                 packet.seqNum, packet.timestamp);
    return VCM_JITTER_BUFFER_ERROR;
  }
  if (ret == kCompleteSession && !retransmitted) {
    
    
    
    timing_->IncomingTimestamp(packet.timestamp, clock_->TimeInMilliseconds());
  }
  if (master_) {
    
    
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Packet seqnum=%u timestamp=%u inserted at %u",
                 packet.seqNum, packet.timestamp,
                 MaskWord64ToUWord32(clock_->TimeInMilliseconds()));
  }
  return VCM_OK;
}

VCMEncodedFrame* VCMReceiver::FrameForDecoding(
    uint16_t max_wait_time_ms,
    int64_t& next_render_time_ms,
    bool render_timing,
    VCMReceiver* dual_receiver) {
  const int64_t start_time_ms = clock_->TimeInMilliseconds();
  uint32_t frame_timestamp = 0;
  
  bool found_frame = jitter_buffer_.NextCompleteTimestamp(
      max_wait_time_ms, &frame_timestamp);

  if (!found_frame) {
    
    const bool dual_receiver_enabled_and_passive = (dual_receiver != NULL &&
        dual_receiver->State() == kPassive &&
        dual_receiver->NackMode() == kNack);
    if (dual_receiver_enabled_and_passive &&
        !jitter_buffer_.CompleteSequenceWithNextFrame()) {
      
      dual_receiver->CopyJitterBufferStateFromReceiver(*this);
    }
    found_frame = jitter_buffer_.NextMaybeIncompleteTimestamp(
        &frame_timestamp);
  }

  if (!found_frame) {
    return NULL;
  }

  
  timing_->SetJitterDelay(jitter_buffer_.EstimatedJitterMs());
  const int64_t now_ms = clock_->TimeInMilliseconds();
  timing_->UpdateCurrentDelay(frame_timestamp);
  next_render_time_ms = timing_->RenderTimeMs(frame_timestamp, now_ms);
  
  bool timing_error = false;
  
  if (next_render_time_ms < 0) {
    timing_error = true;
  } else if (abs(next_render_time_ms - now_ms) > max_video_delay_ms_) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "This frame is out of our delay bounds, resetting jitter "
                 "buffer: %d > %d",
                 static_cast<int>(abs(next_render_time_ms - now_ms)),
                 max_video_delay_ms_);
    timing_error = true;
  } else if (static_cast<int>(timing_->TargetVideoDelay()) >
             max_video_delay_ms_) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "More than %u ms target delay. Flushing jitter buffer and"
                 "resetting timing.", max_video_delay_ms_);
    timing_error = true;
  }

  if (timing_error) {
    
    jitter_buffer_.Flush();
    timing_->Reset();
    return NULL;
  }

  if (!render_timing) {
    
    const int32_t available_wait_time = max_wait_time_ms -
        static_cast<int32_t>(clock_->TimeInMilliseconds() - start_time_ms);
    uint16_t new_max_wait_time = static_cast<uint16_t>(
        VCM_MAX(available_wait_time, 0));
    uint32_t wait_time_ms = timing_->MaxWaitingTime(
        next_render_time_ms, clock_->TimeInMilliseconds());
    if (new_max_wait_time < wait_time_ms) {
      
      
      
      render_wait_event_->Wait(max_wait_time_ms);
      return NULL;
    }
    
    render_wait_event_->Wait(wait_time_ms);
  }

  
  VCMEncodedFrame* frame = jitter_buffer_.ExtractAndSetDecode(frame_timestamp);
  if (frame == NULL) {
    return NULL;
  }
  frame->SetRenderTime(next_render_time_ms);
  TRACE_EVENT_ASYNC_STEP1("webrtc", "Video", frame->TimeStamp(),
                          "SetRenderTS", "render_time", next_render_time_ms);
  if (dual_receiver != NULL) {
    dual_receiver->UpdateState(*frame);
  }
  if (!frame->Complete()) {
    
    bool retransmitted = false;
    const int64_t last_packet_time_ms =
        jitter_buffer_.LastPacketTime(frame, &retransmitted);
    if (last_packet_time_ms >= 0 && !retransmitted) {
      
      
      
      timing_->IncomingTimestamp(frame_timestamp, last_packet_time_ms);
    }
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
}

void VCMReceiver::ReceivedFrameCount(VCMFrameCount* frame_count) const {
  assert(frame_count);
  std::map<FrameType, uint32_t> counts(jitter_buffer_.FrameStatistics());
  frame_count->numDeltaFrames = counts[kVideoFrameDelta];
  frame_count->numKeyFrames = counts[kVideoFrameKey];
}

uint32_t VCMReceiver::DiscardedPackets() const {
  return jitter_buffer_.num_discarded_packets();
}

void VCMReceiver::SetNackMode(VCMNackMode nackMode,
                              int low_rtt_nack_threshold_ms,
                              int high_rtt_nack_threshold_ms) {
  CriticalSectionScoped cs(crit_sect_);
  
  jitter_buffer_.SetNackMode(nackMode, low_rtt_nack_threshold_ms,
                             high_rtt_nack_threshold_ms);
  if (!master_) {
    state_ = kPassive;  
  }
}

void VCMReceiver::SetNackSettings(size_t max_nack_list_size,
                                  int max_packet_age_to_nack,
                                  int max_incomplete_time_ms) {
  jitter_buffer_.SetNackSettings(max_nack_list_size,
                                 max_packet_age_to_nack,
                                 max_incomplete_time_ms);
}

VCMNackMode VCMReceiver::NackMode() const {
  CriticalSectionScoped cs(crit_sect_);
  return jitter_buffer_.nack_mode();
}

VCMNackStatus VCMReceiver::NackList(uint16_t* nack_list,
                                    uint16_t size,
                                    uint16_t* nack_list_length) {
  bool request_key_frame = false;
  uint16_t* internal_nack_list = jitter_buffer_.GetNackList(
      nack_list_length, &request_key_frame);
  if (*nack_list_length > size) {
    *nack_list_length = 0;
    return kNackNeedMoreMemory;
  }
  if (internal_nack_list != NULL && *nack_list_length > 0) {
    memcpy(nack_list, internal_nack_list, *nack_list_length * sizeof(uint16_t));
  }
  if (request_key_frame) {
    return kNackKeyFrameRequest;
  }
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

void VCMReceiver::SetDecodeErrorMode(VCMDecodeErrorMode decode_error_mode) {
  jitter_buffer_.SetDecodeErrorMode(decode_error_mode);
}

VCMDecodeErrorMode VCMReceiver::DecodeErrorMode() const {
  return jitter_buffer_.decode_error_mode();
}

int VCMReceiver::SetMinReceiverDelay(int desired_delay_ms) {
  CriticalSectionScoped cs(crit_sect_);
  if (desired_delay_ms < 0 || desired_delay_ms > kMaxReceiverDelayMs) {
    return -1;
  }
  max_video_delay_ms_ = desired_delay_ms + kMaxVideoDelayMs;
  
  timing_->set_min_playout_delay(desired_delay_ms);
  return 0;
}

int VCMReceiver::RenderBufferSizeMs() {
  uint32_t timestamp_start = 0u;
  uint32_t timestamp_end = 0u;
  
  
  jitter_buffer_.RenderBufferSize(&timestamp_start, &timestamp_end);
  if (timestamp_start == timestamp_end) {
    return 0;
  }
  
  const int64_t now_ms = clock_->TimeInMilliseconds();
  timing_->SetJitterDelay(jitter_buffer_.EstimatedJitterMs());
  
  uint32_t render_start = timing_->RenderTimeMs(timestamp_start, now_ms);
  uint32_t render_end = timing_->RenderTimeMs(timestamp_end, now_ms);
  return render_end - render_start;
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
