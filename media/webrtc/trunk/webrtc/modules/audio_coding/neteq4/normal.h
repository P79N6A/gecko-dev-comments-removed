









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_NORMAL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_NORMAL_H_

#include <string.h>  

#include <vector>

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class BackgroundNoise;
class DecoderDatabase;
class Expand;




class Normal {
 public:
  Normal(int fs_hz, DecoderDatabase* decoder_database,
         const BackgroundNoise& background_noise,
         Expand* expand)
      : fs_hz_(fs_hz),
        decoder_database_(decoder_database),
        background_noise_(background_noise),
        expand_(expand) {
  }

  virtual ~Normal() {}

  
  
  
  
  
  
  
  int Process(const int16_t* input, size_t length,
              Modes last_mode,
              int16_t* external_mute_factor_array,
              AudioMultiVector* output);

 private:
  int fs_hz_;
  DecoderDatabase* decoder_database_;
  const BackgroundNoise& background_noise_;
  Expand* expand_;

  DISALLOW_COPY_AND_ASSIGN(Normal);
};

}  
#endif  
