









#include "video_engine/stream_synchronization.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

enum { kMaxVideoDiffMs = 80 };
enum { kMaxAudioDiffMs = 80 };
enum { kMaxDelay = 1500 };

const float FracMS = 4.294967296E6f;

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

int StreamSynchronization::ComputeDelays(const Measurements& audio,
                                         int current_audio_delay_ms,
                                         int* extra_audio_delay_ms,
                                         const Measurements& video,
                                         int* total_video_delay_target_ms) {
  
  
  
  
  int NTPdiff = (audio.received_ntp_secs - video.received_ntp_secs)
                * 1000;  
  float ntp_diff_frac = audio.received_ntp_frac / FracMS -
        video.received_ntp_frac / FracMS;
  if (ntp_diff_frac > 0.0f)
    NTPdiff += static_cast<int>(ntp_diff_frac + 0.5f);
  else
    NTPdiff += static_cast<int>(ntp_diff_frac - 0.5f);

  int RTCPdiff = (audio.rtcp_arrivaltime_secs - video.rtcp_arrivaltime_secs)
                 * 1000;  
  float rtcp_diff_frac = audio.rtcp_arrivaltime_frac / FracMS -
        video.rtcp_arrivaltime_frac / FracMS;
  if (rtcp_diff_frac > 0.0f)
    RTCPdiff += static_cast<int>(rtcp_diff_frac + 0.5f);
  else
    RTCPdiff += static_cast<int>(rtcp_diff_frac - 0.5f);

  int diff = NTPdiff - RTCPdiff;
  
  if (diff < -1000 || diff > 1000) {
    
    return -1;
  }
  channel_delay_->network_delay = diff;

  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Audio delay is: %d for voice channel: %d",
               current_audio_delay_ms, audio_channel_id_);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Network delay diff is: %d for voice channel: %d",
               channel_delay_->network_delay, audio_channel_id_);
  
  
  int current_diff_ms = *total_video_delay_target_ms - current_audio_delay_ms +
      channel_delay_->network_delay;
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, video_channel_id_,
               "Current diff is: %d for audio channel: %d",
               current_diff_ms, audio_channel_id_);

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
  return 0;
}
}  
