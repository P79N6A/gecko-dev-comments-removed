









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_ACCELERATE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_ACCELERATE_H_

#include <assert.h>

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/time_stretch.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class BackgroundNoise;





class Accelerate : public TimeStretch {
 public:
  Accelerate(int sample_rate_hz, size_t num_channels,
             const BackgroundNoise& background_noise)
      : TimeStretch(sample_rate_hz, num_channels, background_noise) {
  }

  virtual ~Accelerate() {}

  
  
  
  
  
  ReturnCodes Process(const int16_t* input,
                      size_t input_length,
                      AudioMultiVector* output,
                      int16_t* length_change_samples);

 protected:
  
  
  virtual void SetParametersForPassiveSpeech(size_t len,
                                             int16_t* best_correlation,
                                             int* peak_index) const OVERRIDE;

  
  
  virtual ReturnCodes CheckCriteriaAndStretch(
      const int16_t* input, size_t input_length, size_t peak_index,
      int16_t best_correlation, bool active_speech,
      AudioMultiVector* output) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(Accelerate);
};

struct AccelerateFactory {
  AccelerateFactory() {}
  virtual ~AccelerateFactory() {}

  virtual Accelerate* Create(int sample_rate_hz,
                             size_t num_channels,
                             const BackgroundNoise& background_noise) const;
};

}  
#endif  
