













#ifndef WEBRTC_VOICE_ENGINE_UTILITY_H_
#define WEBRTC_VOICE_ENGINE_UTILITY_H_

#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioFrame;

namespace voe {





void RemixAndResample(const AudioFrame& src_frame,
                      PushResampler<int16_t>* resampler,
                      AudioFrame* dst_frame);










void DownConvertToCodecFormat(const int16_t* src_data,
                              int samples_per_channel,
                              int num_channels,
                              int sample_rate_hz,
                              int codec_num_channels,
                              int codec_rate_hz,
                              int16_t* mono_buffer,
                              PushResampler<int16_t>* resampler,
                              AudioFrame* dst_af);

void MixWithSat(int16_t target[],
                int target_channel,
                const int16_t source[],
                int source_channel,
                int source_len);

}  
}  

#endif  
