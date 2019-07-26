




#if !defined(nsWebMDecoder_h_)
#define nsWebMDecoder_h_

#include "nsBuiltinDecoder.h"

class nsWebMDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() {
    if (!nsHTMLMediaElement::IsWebMEnabled()) {
      return nullptr;
    }
    return new nsWebMDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

#endif
