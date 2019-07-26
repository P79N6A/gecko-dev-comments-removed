









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_POST_DECODE_VAD_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_POST_DECODE_VAD_H_

#include <string>  

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/common_types.h"  
#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class PostDecodeVad {
 public:
  PostDecodeVad()
      : enabled_(false),
        running_(false),
        active_speech_(true),
        sid_interval_counter_(0),
        vad_instance_(NULL) {
  }

  virtual ~PostDecodeVad();

  
  void Enable();

  
  void Disable();

  
  void Init();

  
  
  void Update(int16_t* signal, int length,
              AudioDecoder::SpeechType speech_type, bool sid_frame, int fs_hz);

  
  bool enabled() const { return enabled_; }
  bool running() const { return running_; }
  bool active_speech() const { return active_speech_; }

 private:
  static const int kVadMode = 0;  
  
  static const int kVadAutoEnable = 3000;

  bool enabled_;
  bool running_;
  bool active_speech_;
  int sid_interval_counter_;
  ::VadInst* vad_instance_;

  DISALLOW_COPY_AND_ASSIGN(PostDecodeVad);
};

}  
#endif  
