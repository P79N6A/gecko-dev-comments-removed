



#if !defined(nsRawDecoder_h_)
#define nsRawDecoder_h_

#include "nsBuiltinDecoder.h"

namespace mozilla {

class nsRawDecoder : public nsBuiltinDecoder
{
public:
  virtual nsBuiltinDecoder* Clone() {
    if (!IsRawEnabled()) {
      return nullptr;
    }
    return new nsRawDecoder();
  }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
