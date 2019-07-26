









#include "output_mixer_internal.h"

#include "audio_frame_operations.h"
#include "common_audio/resampler/include/resampler.h"
#include "module_common_types.h"
#include "trace.h"

namespace webrtc {
namespace voe {

int RemixAndResample(const AudioFrame& src_frame,
                     Resampler* resampler,
                     AudioFrame* dst_frame) {
  const int16_t* audio_ptr = src_frame.data_;
  int audio_ptr_num_channels = src_frame.num_channels_;
  int16_t mono_audio[AudioFrame::kMaxDataSizeSamples];

  
  if (src_frame.num_channels_ == 2 && dst_frame->num_channels_ == 1) {
    AudioFrameOperations::StereoToMono(src_frame.data_,
                                       src_frame.samples_per_channel_,
                                       mono_audio);
    audio_ptr = mono_audio;
    audio_ptr_num_channels = 1;
  }

  const ResamplerType resampler_type = audio_ptr_num_channels == 1 ?
      kResamplerSynchronous : kResamplerSynchronousStereo;
  if (resampler->ResetIfNeeded(src_frame.sample_rate_hz_,
                               dst_frame->sample_rate_hz_,
                               resampler_type) == -1) {
    *dst_frame = src_frame;
    WEBRTC_TRACE(kTraceError, kTraceVoice, -1,
                "%s ResetIfNeeded failed", __FUNCTION__);
    return -1;
  }

  int out_length = 0;
  if (resampler->Push(audio_ptr,
                      src_frame.samples_per_channel_* audio_ptr_num_channels,
                      dst_frame->data_,
                      AudioFrame::kMaxDataSizeSamples,
                      out_length) == 0) {
    dst_frame->samples_per_channel_ = out_length / audio_ptr_num_channels;
  } else {
    *dst_frame = src_frame;
    WEBRTC_TRACE(kTraceError, kTraceVoice, -1,
                 "%s resampling failed", __FUNCTION__);
    return -1;
  }

  
  if (src_frame.num_channels_ == 1 && dst_frame->num_channels_ == 2) {
    
    
    dst_frame->num_channels_ = 1;
    AudioFrameOperations::MonoToStereo(dst_frame);
  }
  return 0;
}

}  
}  
