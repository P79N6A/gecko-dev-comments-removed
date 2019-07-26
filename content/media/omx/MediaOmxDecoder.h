




#if !defined(MediaOmxDecoder_h_)
#define MediaOmxDecoder_h_

#include "base/basictypes.h"
#include "MediaDecoder.h"

namespace mozilla {

class MediaOmxDecoder : public MediaDecoder
{
public:
  MediaOmxDecoder();
  ~MediaOmxDecoder();

  virtual MediaDecoder* Clone();
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
