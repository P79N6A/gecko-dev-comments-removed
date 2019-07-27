




#ifndef MP3Decoder_h_
#define MP3Decoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class MP3Decoder : public MediaDecoder {
public:
  
  MediaDecoder* Clone() override;
  MediaDecoderStateMachine* CreateStateMachine() override;

  
  
  static bool IsEnabled();
};

} 

#endif
