



#if !defined(RawDecoder_h_)
#define RawDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class RawDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() {
    if (!IsRawEnabled()) {
      return nullptr;
    }
    return new RawDecoder();
  }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
