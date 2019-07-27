





#if !defined(RtspMediaCodecDecoder_h_)
#define RtspMediaCodecDecoder_h_

#include "MediaOmxCommonDecoder.h"

namespace mozilla {

class RtspMediaCodecDecoder MOZ_FINAL : public MediaOmxCommonDecoder
{
public:
  virtual MediaDecoder* Clone() MOZ_OVERRIDE;

  virtual MediaOmxCommonReader* CreateReader() MOZ_OVERRIDE;

  virtual MediaDecoderStateMachine* CreateStateMachine(MediaOmxCommonReader* aReader) MOZ_OVERRIDE;

  virtual void ApplyStateToStateMachine(PlayState aState) MOZ_OVERRIDE;
};

} 

#endif
