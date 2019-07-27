




#if !defined(WebMDecoder_h_)
#define WebMDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class WebMDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() {
    if (!IsWebMEnabled()) {
      return nullptr;
    }
    return new WebMDecoder();
  }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
