









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_DTMF_DETECTION_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_DTMF_DETECTION_H_

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/source/acm_resampler.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class ACMDTMFDetection {
 public:
  ACMDTMFDetection();
  ~ACMDTMFDetection();
  WebRtc_Word16 Enable(ACMCountries cpt = ACMDisableCountryDetection);
  WebRtc_Word16 Disable();
  WebRtc_Word16 Detect(const WebRtc_Word16* in_audio_buff,
                       const WebRtc_UWord16 in_buff_len_word16,
                       const WebRtc_Word32 in_freq_hz,
                       bool& tone_detected,
                       WebRtc_Word16& tone);

 private:
  ACMResampler resampler_;
};

} 

#endif  
