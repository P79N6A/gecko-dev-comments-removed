



#if !defined(nsRawDecoder_h_)
#define nsRawDecoder_h_

#include "nsBuiltinDecoder.h"

class nsRawDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() { 
    if (!nsHTMLMediaElement::IsRawEnabled()) {
      return nullptr;
    }    
    return new nsRawDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

#endif
