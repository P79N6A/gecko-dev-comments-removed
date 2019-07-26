









#ifndef WEBRTC_VOICE_ENGINE_OUTPUT_MIXER_INTERNAL_H_
#define WEBRTC_VOICE_ENGINE_OUTPUT_MIXER_INTERNAL_H_

namespace webrtc {

class AudioFrame;
class Resampler;

namespace voe {






int RemixAndResample(const AudioFrame& src_frame,
                     Resampler* resampler,
                     AudioFrame* dst_frame);

}  
}  

#endif  
