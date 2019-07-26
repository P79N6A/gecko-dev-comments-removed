









#include "webrtc/modules/audio_coding/neteq4/post_decode_vad.h"

namespace webrtc {

PostDecodeVad::~PostDecodeVad() {
  if (vad_instance_)
    WebRtcVad_Free(vad_instance_);
}

void PostDecodeVad::Enable() {
  if (!vad_instance_) {
    
    if (WebRtcVad_Create(&vad_instance_) != 0) {
      
      Disable();
      return;
    }
  }
  Init();
  enabled_ = true;
}

void PostDecodeVad::Disable() {
  enabled_ = false;
  running_ = false;
}

void PostDecodeVad::Init() {
  running_ = false;
  if (vad_instance_) {
    WebRtcVad_Init(vad_instance_);
    WebRtcVad_set_mode(vad_instance_, kVadMode);
    running_ = true;
  }
}

void PostDecodeVad::Update(int16_t* signal, int length,
                           AudioDecoder::SpeechType speech_type,
                           bool sid_frame,
                           int fs_hz) {
  if (!vad_instance_ || !enabled_) {
    return;
  }

  if (speech_type == AudioDecoder::kComfortNoise || sid_frame ||
      fs_hz > 16000) {
    
    running_ = false;
    active_speech_ = true;
    sid_interval_counter_ = 0;
  } else if (!running_) {
    ++sid_interval_counter_;
  }

  if (sid_interval_counter_ >= kVadAutoEnable) {
    Init();
  }

  if (length > 0 && running_) {
    int vad_sample_index = 0;
    active_speech_ = false;
    
    for (int vad_frame_size_ms = 30; vad_frame_size_ms >= 10;
        vad_frame_size_ms -= 10) {
      int vad_frame_size_samples = vad_frame_size_ms * fs_hz / 1000;
      while (length - vad_sample_index >= vad_frame_size_samples) {
        int vad_return = WebRtcVad_Process(
            vad_instance_, fs_hz, &signal[vad_sample_index],
            vad_frame_size_samples);
        active_speech_ |= (vad_return == 1);
        vad_sample_index += vad_frame_size_samples;
      }
    }
  }
}

}  
