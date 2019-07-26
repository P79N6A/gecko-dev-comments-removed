









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_RESAMPLER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_RESAMPLER_H_

#include "resampler.h"
#include "typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;

class ACMResampler {
 public:
  ACMResampler();
  ~ACMResampler();

  WebRtc_Word16 Resample10Msec(const WebRtc_Word16* inAudio,
                               const WebRtc_Word32 inFreqHz,
                               WebRtc_Word16* outAudio,
                               const WebRtc_Word32 outFreqHz,
                               WebRtc_UWord8 numAudioChannels);

 private:
  
  Resampler _resampler;
  CriticalSectionWrapper* _resamplerCritSect;
};

}  

#endif  
