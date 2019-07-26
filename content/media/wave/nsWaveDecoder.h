




#if !defined(nsWaveDecoder_h_)
#define nsWaveDecoder_h_

#include "nsBuiltinDecoder.h"












namespace mozilla {

class nsWaveDecoder : public nsBuiltinDecoder
{
public:
  virtual nsBuiltinDecoder* Clone() {
    if (!IsWaveEnabled()) {
      return nullptr;
    }
    return new nsWaveDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
