









#ifndef WEBRTC_COMMON_AUDIO_AUDIO_CONVERTER_H_
#define WEBRTC_COMMON_AUDIO_AUDIO_CONVERTER_H_


#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_processing/common.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_vector.h"

namespace webrtc {

class PushSincResampler;







class AudioConverter {
 public:
  AudioConverter(int src_channels, int src_frames,
                 int dst_channels, int dst_frames);

  void Convert(const float* const* src,
               int src_channels,
               int src_frames,
               int dst_channels,
               int dst_frames,
               float* const* dest);

 private:
  const int src_channels_;
  const int src_frames_;
  const int dst_channels_;
  const int dst_frames_;
  scoped_ptr<ChannelBuffer<float>> downmix_buffer_;
  ScopedVector<PushSincResampler> resamplers_;

  DISALLOW_COPY_AND_ASSIGN(AudioConverter);
};

}  

#endif  
