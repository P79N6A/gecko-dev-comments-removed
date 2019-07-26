




#if !defined(nsWebMDecoder_h_)
#define nsWebMDecoder_h_

#include "nsBuiltinDecoder.h"

class nsWebMDecoder : public nsBuiltinDecoder
{
public:
  virtual nsBuiltinDecoder* Clone() {
    if (!IsWebMEnabled()) {
      return nullptr;
    }
    return new nsWebMDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

#endif
