





#if !defined(RtspMediaCodecDecoder_h_)
#define RtspMediaCodecDecoder_h_

#include "MediaOmxCommonDecoder.h"

namespace mozilla {

class RtspMediaCodecDecoder final : public MediaOmxCommonDecoder
{
public:
  virtual MediaDecoder* Clone() override;

  virtual MediaOmxCommonReader* CreateReader() override;

  virtual MediaDecoderStateMachine* CreateStateMachineFromReader(MediaOmxCommonReader* aReader) override;

  virtual void ChangeState(PlayState aState) override;
};

} 

#endif
