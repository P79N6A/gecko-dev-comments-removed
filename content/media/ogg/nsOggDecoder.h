




#if !defined(nsOggDecoder_h_)
#define nsOggDecoder_h_

#include "nsBuiltinDecoder.h"

namespace mozilla {

class nsOggDecoder : public nsBuiltinDecoder
{
public:
  virtual nsBuiltinDecoder* Clone() {
    if (!IsOggEnabled()) {
      return nullptr;
    }
    return new nsOggDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
