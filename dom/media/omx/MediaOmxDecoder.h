




#if !defined(MediaOmxDecoder_h_)
#define MediaOmxDecoder_h_

#include "MediaOmxCommonDecoder.h"

namespace mozilla {

class MediaOmxDecoder : public MediaOmxCommonDecoder
{
public:
  virtual MediaDecoder* Clone();
  virtual MediaOmxCommonReader* CreateReader();
  virtual MediaDecoderStateMachine* CreateStateMachineFromReader(MediaOmxCommonReader* aReader);
};

} 

#endif
