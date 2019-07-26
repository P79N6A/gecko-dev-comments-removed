









#include "webrtc/video_engine/stream_synchronization.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <algorithm>

#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

static const int kMaxChangeMs = 80;
static const int kMaxDeltaDelayMs = 10000;
static const int kFilterLength = 4;

static const int kMinDeltaMs = 30;

struct ViESyncDelay {
  ViESyncDelay() {
    extra_video_delay_ms = 0;
    last_video_delay_ms = 0;
    extra_audio_delay_ms = 0;
    last_audio_delay_ms = 0;
    network_delay = 120;
  }

  int extra_video_delay_ms;
  int last_video_delay_ms;
  int extra_audio_delay_ms;
  int last_audio_delay_ms;
  int network_delay;
};

StreamSynchronization::StreamSynchronization(int audio_channel_id,
                                             int video_channel_id)
    : channel_delay_(new ViESyncDelay),
      audio_channel_id_(audio_channel_id),
      video_channel_id_(video_channel_id),
      base_target_delay_ms_(0),
      avg_diff_ms_(0) {}

StreamSynchronization::~StreamSynchronization() {
  delete channel_delay_;
}

bool StreamSynchronization::ComputeRelativeDelay(
    const Measurements& audio_measurement,
    const Measurements& video_measurement,
    int* relative_delay_ms) {
  assert(relative_delay_ms);
  if (audio_measurement.rtcp.size() < 2 || video_measurement.rtcp.size() < 2) {
    
    return false;
  }
  int64_t audio_last_capture_time_ms;
  if (!synchronization::RtpToNtpMs(audio_measurement.latest_timestamp,
                                   audio_measurement.rtcp,
                                   &audio_last_capture_time_ms)) {
    return false;
  }
  int64_t video_last_capture_time_ms;
  if (!synchronization::RtpToNtpMs(video_measurement.latest_timestamp,
                                   video_measurement.rtcp,
                                   &video_last_capture_time_ms)) {
    return false;
  }
  if (video_last_capture_time_ms < 0) {
    return false;
  }
  
  *relative_delay_ms = video_measurement.latest_receive_time_ms -
      audio_measurement.latest_receive_time_ms -
      (video_last_capture_time_ms - audio_last_capture_time_ms);
  if (*relative_delay_ms > kMaxDeltaDelayMs ||
      *relative_delay_ms < -kMaxDeltaDelayMs) {
    return false;
  }
  return true;
}

bool StreamSynchronization::ComputeDelays(int relative_delay_ms,
                                          int current_audio_delay_ms,
                                          int* total_audio_delay_target_ms,
                                          int* total_video_delay_target_ms) {
  assert(total_audio_delay_target_ms && total_video_delay_target_ms);

  int current_video_delay_ms = *total_video_delay_target_ms;
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Audio delay is: %d for voice channel: %d",
               current_audio_delay_ms, audio_channel_id_);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Network delay diff is: %d for voice channel: %d",
               channel_delay_->network_delay, audio_channel_id_);
  
  
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Current diff is: %d for audio channel: %d",
               relative_delay_ms, audio_channel_id_);

  int current_diff_ms = current_video_delay_ms - current_audio_delay_ms +
      relative_delay_ms;

  avg_diff_ms_ = ((kFilterLength - 1) * avg_diff_ms_ +
      current_diff_ms) / kFilterLength;
  if (abs(avg_diff_ms_) < kMinDeltaMs) {
    
    return false;
  }

  
  int diff_ms = avg_diff_ms_ / 2;
  diff_ms = std::min(diff_ms, kMaxChangeMs);
  diff_ms = std::max(diff_ms, -kMaxChangeMs);

  
  avg_diff_ms_ = 0;

  if (diff_ms > 0) {
    
    
    if (channel_delay_->extra_video_delay_ms > base_target_delay_ms_) {
      
      
      channel_delay_->extra_video_delay_ms -= diff_ms;
      channel_delay_->extra_audio_delay_ms = base_target_delay_ms_;
    } else {  
      
      channel_delay_->extra_audio_delay_ms += diff_ms;
      channel_delay_->extra_video_delay_ms = base_target_delay_ms_;
    }
  } else {  
    
    
    if (channel_delay_->extra_audio_delay_ms > base_target_delay_ms_) {
      
      
      
      channel_delay_->extra_audio_delay_ms += diff_ms;
      channel_delay_->extra_video_delay_ms = base_target_delay_ms_;
    } else {  
      
      
      channel_delay_->extra_video_delay_ms -= diff_ms;  
      channel_delay_->extra_audio_delay_ms = base_target_delay_ms_;
    }
  }

  
  channel_delay_->extra_video_delay_ms = std::max(
      channel_delay_->extra_video_delay_ms, base_target_delay_ms_);

  int new_video_delay_ms;
  if (channel_delay_->extra_video_delay_ms > base_target_delay_ms_) {
    new_video_delay_ms = channel_delay_->extra_video_delay_ms;
  } else {
    
    
    new_video_delay_ms = channel_delay_->last_video_delay_ms;
  }

  
  new_video_delay_ms = std::max(
      new_video_delay_ms, channel_delay_->extra_video_delay_ms);

  
  new_video_delay_ms =
      std::min(new_video_delay_ms, base_target_delay_ms_ + kMaxDeltaDelayMs);

  int new_audio_delay_ms;
  if (channel_delay_->extra_audio_delay_ms > base_target_delay_ms_) {
    new_audio_delay_ms = channel_delay_->extra_audio_delay_ms;
  } else {
    
    
    new_audio_delay_ms = channel_delay_->last_audio_delay_ms;
  }

  
  new_audio_delay_ms = std::max(
      new_audio_delay_ms, channel_delay_->extra_audio_delay_ms);

  
  new_audio_delay_ms =
      std::min(new_audio_delay_ms, base_target_delay_ms_ + kMaxDeltaDelayMs);

  
  channel_delay_->last_video_delay_ms = new_video_delay_ms;
  channel_delay_->last_audio_delay_ms = new_audio_delay_ms;

  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
      "Sync video delay %d ms for video channel and audio delay %d for audio "
      "channel %d",
      new_video_delay_ms, channel_delay_->extra_audio_delay_ms,
      audio_channel_id_);

  
  *total_video_delay_target_ms = new_video_delay_ms;
  *total_audio_delay_target_ms = new_audio_delay_ms;
  return true;
}

void StreamSynchronization::SetTargetBufferingDelay(int target_delay_ms) {
  
  channel_delay_->extra_audio_delay_ms +=
      target_delay_ms - base_target_delay_ms_;
  channel_delay_->last_audio_delay_ms +=
      target_delay_ms - base_target_delay_ms_;

  
  
  channel_delay_->last_video_delay_ms +=
      target_delay_ms - base_target_delay_ms_;

  channel_delay_->extra_video_delay_ms +=
      target_delay_ms - base_target_delay_ms_;

  
  base_target_delay_ms_ = target_delay_ms;
}

}  
