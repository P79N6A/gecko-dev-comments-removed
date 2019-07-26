









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_RESAMPLER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_RESAMPLER_H_

#include "webrtc/modules/audio_processing/aec/aec_core.h"

enum {
  kResamplingDelay = 1
};
enum {
  kResamplerBufferSize = FRAME_LEN * 4
};


int WebRtcAec_CreateResampler(void** resampInst);
int WebRtcAec_InitResampler(void* resampInst, int deviceSampleRateHz);
int WebRtcAec_FreeResampler(void* resampInst);


int WebRtcAec_GetSkew(void* resampInst, int rawSkew, float* skewEst);


void WebRtcAec_ResampleLinear(void* resampInst,
                              const short* inspeech,
                              int size,
                              float skew,
                              short* outspeech,
                              int* size_out);

#endif  
