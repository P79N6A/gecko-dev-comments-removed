









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DTMF_TONE_GENERATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DTMF_TONE_GENERATOR_H_


#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DtmfToneGenerator {
 public:
  enum ReturnCodes {
    kNotInitialized = -1,
    kParameterError = -2,
    kStereoNotSupported = -3,
  };

  DtmfToneGenerator();
  virtual ~DtmfToneGenerator() {}
  virtual int Init(int fs, int event, int attenuation);
  virtual void Reset();
  virtual int Generate(int num_samples, AudioMultiVector* output);
  virtual bool initialized() const { return initialized_; }

 private:
  static const int kCoeff1[4][16];  
  static const int kCoeff2[4][16];  
  static const int kInitValue1[4][16];  
  static const int kInitValue2[4][16];  
  static const int kAmplitude[37];  
  static const int16_t kAmpMultiplier = 23171;  

  bool initialized_;            
  int coeff1_;                  
  int coeff2_;                  
  int amplitude_;               
  int16_t sample_history1_[2];  
  int16_t sample_history2_[2];  

  DISALLOW_COPY_AND_ASSIGN(DtmfToneGenerator);
};

}  
#endif
