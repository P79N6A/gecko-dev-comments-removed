




#if !defined(nsWaveDecoder_h_)
#define nsWaveDecoder_h_

#include "nsBuiltinDecoder.h"













class nsWaveDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() {
    if (!nsHTMLMediaElement::IsWaveEnabled()) {
      return nullptr;
    }
    return new nsWaveDecoder();
  }
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
