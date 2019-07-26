









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PREEMPTIVE_EXPAND_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PREEMPTIVE_EXPAND_H_

#include <assert.h>

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/time_stretch.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class BackgroundNoise;





class PreemptiveExpand : public TimeStretch {
 public:
  PreemptiveExpand(int sample_rate_hz, size_t num_channels,
                   const BackgroundNoise& background_noise)
      : TimeStretch(sample_rate_hz, num_channels, background_noise),
        old_data_length_per_channel_(-1),
        overlap_samples_(5 * sample_rate_hz / 8000) {
  }

  virtual ~PreemptiveExpand() {}

  
  
  
  
  
  ReturnCodes Process(const int16_t *pw16_decoded,
                      int len,
                      int old_data_len,
                      AudioMultiVector* output,
                      int16_t* length_change_samples);

 protected:
  
  
  virtual void SetParametersForPassiveSpeech(size_t len,
                                             int16_t* w16_bestCorr,
                                             int* w16_bestIndex) const;

  
  
  virtual ReturnCodes CheckCriteriaAndStretch(
      const int16_t *pw16_decoded, size_t len, size_t w16_bestIndex,
      int16_t w16_bestCorr, bool w16_VAD,
      AudioMultiVector* output) const;

 private:
  int old_data_length_per_channel_;
  int overlap_samples_;

  DISALLOW_COPY_AND_ASSIGN(PreemptiveExpand);
};

struct PreemptiveExpandFactory {
  PreemptiveExpandFactory() {}
  virtual ~PreemptiveExpandFactory() {}

  virtual PreemptiveExpand* Create(
      int sample_rate_hz,
      size_t num_channels,
      const BackgroundNoise& background_noise) const;
};

}  
#endif  
