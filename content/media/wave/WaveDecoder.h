




#if !defined(WaveDecoder_h_)
#define WaveDecoder_h_

#include "MediaDecoder.h"












namespace mozilla {

class WaveDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() {
    if (!IsWaveEnabled()) {
      return nullptr;
    }
    return new WaveDecoder();
  }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
