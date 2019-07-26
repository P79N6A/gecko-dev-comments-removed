



#ifndef __AppleDecoder_h__
#define __AppleDecoder_h__

#include "MediaDecoder.h"

namespace mozilla {

class AppleDecoder : public MediaDecoder
{
public:
  AppleDecoder();

  virtual MediaDecoder* Clone() MOZ_OVERRIDE;
  virtual MediaDecoderStateMachine* CreateStateMachine() MOZ_OVERRIDE;

};

}

#endif 
