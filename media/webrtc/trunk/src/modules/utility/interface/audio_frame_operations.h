









#ifndef WEBRTC_VOICE_ENGINE_AUDIO_FRAME_OPERATIONS_H_
#define WEBRTC_VOICE_ENGINE_AUDIO_FRAME_OPERATIONS_H_

#include "typedefs.h"

namespace webrtc {

class AudioFrame;




class AudioFrameOperations {
 public:
  
  
  
  
  static void MonoToStereo(const int16_t* src_audio, int samples_per_channel,
                           int16_t* dst_audio);
  
  
  static int MonoToStereo(AudioFrame* frame);

  
  
  
  static void StereoToMono(const int16_t* src_audio, int samples_per_channel,
                           int16_t* dst_audio);
  
  
  static int StereoToMono(AudioFrame* frame);

  
  
  static void SwapStereoChannels(AudioFrame* frame);

  
  static void Mute(AudioFrame& frame);

  static int Scale(float left, float right, AudioFrame& frame);

  static int ScaleWithSat(float scale, AudioFrame& frame);
};

}  

#endif  
