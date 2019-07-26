




#if !defined(OggDecoder_h_)
#define OggDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class OggDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() {
    if (!IsOggEnabled()) {
      return nullptr;
    }
    return new OggDecoder();
  }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
