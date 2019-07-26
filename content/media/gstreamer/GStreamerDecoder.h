





#if !defined(GStreamerDecoder_h_)
#define GStreamerDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class GStreamerDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() { return new GStreamerDecoder(); }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
