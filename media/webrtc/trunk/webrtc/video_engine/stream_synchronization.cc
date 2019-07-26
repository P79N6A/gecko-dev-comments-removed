









#include "video_engine/stream_synchronization.h"

#include <assert.h>
#include <algorithm>
#include <cmath>

#include "system_wrappers/interface/trace.h"

namespace webrtc {

const int kMaxVideoDiffMs = 80;
const int kMaxAudioDiffMs = 80;
const int kMaxDelay = 1500;

struct ViESyncDelay {
  ViESyncDelay() {
    extra_video_delay_ms = 0;
    last_video_delay_ms = 0;
    extra_audio_delay_ms = 0;
    last_sync_delay = 0;
    network_delay = 120;
  }

  int extra_video_delay_ms;
  int last_video_delay_ms;
  int extra_audio_delay_ms;
  int last_sync_delay;
  int network_delay;
};

StreamSynchronization::StreamSynchronization(int audio_channel_id,
                                             int video_channel_id)
    : channel_delay_(new ViESyncDelay),
      audio_channel_id_(audio_channel_id),
      video_channel_id_(video_channel_id) {}

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
  if (*relative_delay_ms > 1000 || *relative_delay_ms < -1000) {
    return false;
  }
  return true;
}

bool StreamSynchronization::ComputeDelays(int relative_delay_ms,
                                          int current_audio_delay_ms,
                                          int* extra_audio_delay_ms,
                                          int* total_video_delay_target_ms) {
  assert(extra_audio_delay_ms && total_video_delay_target_ms);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Audio delay is: %d for voice channel: %d",
               current_audio_delay_ms, audio_channel_id_);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Network delay diff is: %d for voice channel: %d",
               channel_delay_->network_delay, audio_channel_id_);
  
  
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Current diff is: %d for audio channel: %d",
               relative_delay_ms, audio_channel_id_);

  int current_diff_ms = *total_video_delay_target_ms - current_audio_delay_ms +
      relative_delay_ms;

  int video_delay_ms = 0;
  if (current_diff_ms > 0) {
    
    
    
    if (channel_delay_->extra_video_delay_ms > 0) {
      
      

      
      video_delay_ms = *total_video_delay_target_ms;

      
      if (video_delay_ms <
          channel_delay_->last_video_delay_ms - kMaxVideoDiffMs) {
        video_delay_ms =
            channel_delay_->last_video_delay_ms - kMaxVideoDiffMs;
        channel_delay_->extra_video_delay_ms =
            video_delay_ms - *total_video_delay_target_ms;
      } else {
        channel_delay_->extra_video_delay_ms = 0;
      }
      channel_delay_->last_video_delay_ms = video_delay_ms;
      channel_delay_->last_sync_delay = -1;
      channel_delay_->extra_audio_delay_ms = 0;
    } else {  
      
      if (channel_delay_->last_sync_delay >= 0) {
        
        int audio_diff_ms = current_diff_ms / 2;
        if (audio_diff_ms > kMaxAudioDiffMs) {
          
          
          audio_diff_ms = kMaxAudioDiffMs;
        }
        
        channel_delay_->extra_audio_delay_ms += audio_diff_ms;

        
        if (channel_delay_->extra_audio_delay_ms > kMaxDelay) {
          channel_delay_->extra_audio_delay_ms = kMaxDelay;
        }

        
        video_delay_ms = *total_video_delay_target_ms;
        channel_delay_->extra_video_delay_ms = 0;
        channel_delay_->last_video_delay_ms = video_delay_ms;
        channel_delay_->last_sync_delay = 1;
      } else {  
        
        
        channel_delay_->extra_audio_delay_ms = 0;
        
        video_delay_ms = *total_video_delay_target_ms;
        channel_delay_->extra_video_delay_ms = 0;
        channel_delay_->last_video_delay_ms = video_delay_ms;
        channel_delay_->last_sync_delay = 0;
      }
    }
  } else {  
    
    
    

    if (channel_delay_->extra_audio_delay_ms > 0) {
      
      
      int audio_diff_ms = current_diff_ms / 2;
      if (audio_diff_ms < -1 * kMaxAudioDiffMs) {
        
        audio_diff_ms = -1 * kMaxAudioDiffMs;
      }
      
      channel_delay_->extra_audio_delay_ms += audio_diff_ms;

      if (channel_delay_->extra_audio_delay_ms < 0) {
        
        channel_delay_->extra_audio_delay_ms = 0;
        channel_delay_->last_sync_delay = 0;
      } else {
        
        channel_delay_->last_sync_delay = 1;
      }

      
      video_delay_ms = *total_video_delay_target_ms;
      channel_delay_->extra_video_delay_ms = 0;
      channel_delay_->last_video_delay_ms = video_delay_ms;
    } else {  
      
      channel_delay_->extra_audio_delay_ms = 0;

      
      int video_diff_ms = -1 * current_diff_ms;

      
      video_delay_ms = *total_video_delay_target_ms + video_diff_ms;
      if (video_delay_ms > channel_delay_->last_video_delay_ms) {
        if (video_delay_ms >
            channel_delay_->last_video_delay_ms + kMaxVideoDiffMs) {
          
          video_delay_ms =
              channel_delay_->last_video_delay_ms + kMaxVideoDiffMs;
        }
        
        if (video_delay_ms > kMaxDelay) {
          video_delay_ms = kMaxDelay;
        }
      } else {
        if (video_delay_ms <
            channel_delay_->last_video_delay_ms - kMaxVideoDiffMs) {
          
          video_delay_ms =
              channel_delay_->last_video_delay_ms - kMaxVideoDiffMs;
        }
        
        if (video_delay_ms < *total_video_delay_target_ms) {
          video_delay_ms = *total_video_delay_target_ms;
        }
      }
      
      channel_delay_->extra_video_delay_ms =
          video_delay_ms - *total_video_delay_target_ms;
      channel_delay_->last_video_delay_ms = video_delay_ms;
      channel_delay_->last_sync_delay = -1;
    }
  }

  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
      "Sync video delay %d ms for video channel and audio delay %d for audio "
      "channel %d",
      video_delay_ms, channel_delay_->extra_audio_delay_ms, audio_channel_id_);

  *extra_audio_delay_ms = channel_delay_->extra_audio_delay_ms;

  if (video_delay_ms < 0) {
    video_delay_ms = 0;
  }
  *total_video_delay_target_ms =
      (*total_video_delay_target_ms  >  video_delay_ms) ?
      *total_video_delay_target_ms : video_delay_ms;
  return true;
}
}  
