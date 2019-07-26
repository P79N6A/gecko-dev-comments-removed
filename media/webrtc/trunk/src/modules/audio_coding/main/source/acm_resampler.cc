









#include <string.h>

#include "acm_resampler.h"

#include "critical_section_wrapper.h"
#include "resampler.h"
#include "signal_processing_library.h"
#include "trace.h"

namespace webrtc {

ACMResampler::ACMResampler()
    : _resamplerCritSect(CriticalSectionWrapper::CreateCriticalSection()) {
}

ACMResampler::~ACMResampler() {
  delete _resamplerCritSect;
}

WebRtc_Word16 ACMResampler::Resample10Msec(const WebRtc_Word16* inAudio,
                                           WebRtc_Word32 inFreqHz,
                                           WebRtc_Word16* outAudio,
                                           WebRtc_Word32 outFreqHz,
                                           WebRtc_UWord8 numAudioChannels) {
  CriticalSectionScoped cs(_resamplerCritSect);

  if (inFreqHz == outFreqHz) {
    size_t length = static_cast<size_t>(inFreqHz * numAudioChannels / 100);
    memcpy(outAudio, inAudio, length * sizeof(WebRtc_Word16));
    return static_cast<WebRtc_Word16>(inFreqHz / 100);
  }

  
  int maxLen = 480 * numAudioChannels;
  int lengthIn = (WebRtc_Word16)(inFreqHz / 100) * numAudioChannels;
  int outLen;

  WebRtc_Word32 ret;
  ResamplerType type;
  type = (numAudioChannels == 1) ? kResamplerSynchronous :
      kResamplerSynchronousStereo;

  ret = _resampler.ResetIfNeeded(inFreqHz, outFreqHz, type);
  if (ret < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, 0,
                 "Error in reset of resampler");
    return -1;
  }

  ret = _resampler.Push(inAudio, lengthIn, outAudio, maxLen, outLen);
  if (ret < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, 0,
                 "Error in resampler: resampler.Push");
    return -1;
  }

  WebRtc_Word16 outAudioLenSmpl = (WebRtc_Word16) outLen / numAudioChannels;

  return outAudioLenSmpl;
}

}  
