




#if !defined(nsOggDecoder_h_)
#define nsOggDecoder_h_

#include "nsBuiltinDecoder.h"

class nsOggDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() {
    if (!nsHTMLMediaElement::IsOggEnabled()) {
      return nullptr;
    }
    return new nsOggDecoder();
  }
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
