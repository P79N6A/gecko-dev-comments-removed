









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DTMF_TONE_GENERATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DTMF_TONE_GENERATOR_H_

#include "webrtc/modules/audio_coding/neteq4/dtmf_tone_generator.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockDtmfToneGenerator : public DtmfToneGenerator {
 public:
  virtual ~MockDtmfToneGenerator() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_METHOD3(Init,
      int(int fs, int event, int attenuation));
  MOCK_METHOD0(Reset,
      void());
  MOCK_METHOD2(Generate,
      int(int num_samples, AudioMultiVector* output));
  MOCK_CONST_METHOD0(initialized,
      bool());
};

}  
#endif  
